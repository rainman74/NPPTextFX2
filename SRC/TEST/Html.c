#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <alloc.h>
#include <process.h>
#include <time.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>

// This C file is used to develop and debug the highly detailed C functions.
// It contains some of the original samples and testing tools
// It is not expected to compile because it is adapted each time it is needed
// for whatever function is being tested.
// and if later changes are made in Dev-C++, this code goes out of date
//
// Compiles in Borland C++ 3.1 DOS

#ifndef NELEM
#define NELEM(xyzzy) (sizeof(xyzzy)/sizeof(xyzzy[0]))
#endif

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

#ifdef PLAT_WIN
#define NPPWINDOWS 1
#else
#define NPPWINDOWS 0
#endif
#define NPPDEBUG 1 /* set to 1 to turn on a couple of checkers */

#define DWORD unsigned long
#define LPCVOID const void *
#define LPCSTR const char *
#define BOOL int
#define TEXT(x) x
#define FALSE 0
#define TRUE 1

#pragma warn -rch /* Borland-C warning: unreachable code */
#pragma warn -ccc /* Borland-C warning: condition is always true/false */
#pragma warn -pia /* Borland-C warning: possibly incorrect assignment */

#define HWND int
struct NppData {
  HWND _nppHandle;
  HWND _scintillaMainHandle;
  HWND _scintillaSecondHandle;
} nppData;
#define PLUGIN_NAME "TextFX (debug)"
#define MB_OK 0
#define MB_ICONSTOP 0
#define MB_ICONWARNING 0
#define MB_ICONINFORMATION 0
#define snprintf snprintfX
size_t snprintfX(char *buffer,size_t buffersz,const char *format,...);
#define TCHAR char
#define LPCTSTR char *
#define UINT unsigned
#define highperformance 1
#define QSORT_FCMP __cdecl

#pragma warn -par /* Borland-C warning: parameter is never used */
int MessageBox(int hWnd,const char *lpText,const char *lpCaption,int uType) {
  printf("%s: %s\n",lpCaption,lpText);
  return(0);
}

#pragma warn +par /* Borland-C warning: parameter is never used */

/* Borland-C for DOS doesn't have vsnprintf() and we need one to debug routines using it */
//#define _MSC_VER /* define one of these to test each libraries vsnprintf() bugs */
//#define __MINGW32__
//#define __WATCOMC__
//#define __DMC__
//The Borland-C libarary didn't have any bugs so rem them all to test
size_t vsnprintf(char *buf,size_t bufl,const char *format,va_list ap) {
  char mybuf[1024];
  size_t rv;

if (bufl>=sizeof(mybuf)) {
  buf[bufl-1]='\0';
  rv=vsprintf(buf,format,ap);
  if (buf[bufl-1]!='\0') {fprintf(stderr,"Stack Destroyed! %u wasn't large enough for the buffer\b\n",bufl); exit(1); }
} else {
  mybuf[sizeof(mybuf)-1]='\0';
  rv=vsprintf(mybuf,format,ap);
  if (mybuf[sizeof(mybuf)-1]!='\0') {fprintf(stderr,"Stack Destroyed! %u wasn't large enough for the buffer\b\n",sizeof(mybuf)); exit(1); }
  if (rv>=bufl) {
    if (bufl) strncpy(buf,mybuf,bufl);
#ifdef __WATCOMC__
    //if (rv==bufl) buf[--rv]='\0';
#endif
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__DMC__)
    rv=(unsigned)-1;
#endif
    //buf[bufl-1]='\0'; /* I don't know which compilers are bugged this way */
  } else strcpy(buf,mybuf);
}
  return(rv);
}

// don't delete or replace any functions above this line

// SECTION: Beginning of library functions

#if NPPDEBUG
/* We control all malloc/realloc/free buffers. We add 3 bytes to the buffer size and place
   1 check code at the beginning and 2 at the end. If any of those are modifed then we know
   that the caller has gone beyond the bounds. */
struct _MALLOC_STACK {
  void *buf;
  unsigned len;
  const char *title,*dummy; /* dummy makes it 16 bytes */
} g_mallocstack[100];

#define SAFEBEGIN sizeof(int) /* resulting buffers must be DWORD aligned if the originals are */
#define SAFEEND sizeof(int)
// wants the fake buf and fake ct
// returns the real buf
EXTERNC void *freesafebounds(void *bf,unsigned ct,const char *title) {
  if (bf) {
    unsigned i;
    unsigned char *bft=(unsigned char *)bf;
    bft-=SAFEBEGIN;
    for(i=0; i<SAFEBEGIN; i++) if (bft[i]!=255-i) {MessageBox(0,"Buffer write beyond begin",title, MB_OK|MB_ICONSTOP); break;}
    //if (bft[0] != 255)  MessageBox(0,"Buffer went beyond buffer begin",title, MB_OK|MB_ICONSTOP);
    for(i=0; i<SAFEEND; i++) if (bft[ct+SAFEBEGIN+i]!=255-i) {MessageBox(0,"Buffer write beyond end",title, MB_OK|MB_ICONSTOP); break;}
    //if (bft[ct+1]!=255) MessageBox(0,"Buffer went beyond wrote 1+ buffer end",title, MB_OK|MB_ICONSTOP);
    //if (bft[ct+2]!=254 )  MessageBox(0,"Buffer went beyond wrote 2+ buffer end",title, MB_OK|MB_ICONSTOP);
    bf = (void *)bft;
  }
  return(bf);
}

// wants the real buf and fake ct
// returns the fake buf
EXTERNC void *reallocsafebounds(void *bf,unsigned ct) {
  void *bf2;
  bf2=bf?realloc(bf,ct+(SAFEBEGIN+SAFEEND)):malloc(ct+(SAFEBEGIN+SAFEEND));
  if (bf2) {
    unsigned char *bft2=(unsigned char *)bf2;
    unsigned i; for(i=0; i<SAFEBEGIN; i++) bft2[i]=255-i;
    //bft2[0]   =255;
    for(i=0; i<SAFEEND; i++) bft2[ct+SAFEBEGIN+i]=255-i;
    //bft2[ct+1]=255;
    //bft2[ct+2]=254;
    bft2+=SAFEBEGIN;
    bf2 = (void *)bft2;
    //if (!bf) memset(bf2,'@',ct);
  }
  return(bf2);
}

EXTERNC void freesafe(void *bf,const char *title) {
  if (bf) {
    int i; for(i=0; i<NELEM(g_mallocstack); i++) if (g_mallocstack[i].buf==bf) {
      free(freesafebounds(g_mallocstack[i].buf,g_mallocstack[i].len,title));
      //g_mallocstack[i].buf=NULL;
      memset(g_mallocstack+i,0,sizeof(g_mallocstack[0]));
      return;
    }
    MessageBox(0,"free() buffer never allocated",title, MB_OK|MB_ICONSTOP);
    return;
  }
  MessageBox(0,"free() NULL buffer",title, MB_OK|MB_ICONSTOP);
}

EXTERNC void *reallocsafeX(void *bf,unsigned ct,char *title,int allownull) {
  void *bf2=NULL;
  if (ct) {
    if (bf) {
      int i; for(i=0; i<NELEM(g_mallocstack); i++) if (g_mallocstack[i].buf==bf) goto fnd;
      MessageBox(0,"Buffer realloc() not originally realloc()",title, MB_OK|MB_ICONSTOP);
      return(NULL);
      if (0) {
fnd:    if (ct==g_mallocstack[i].len) MessageBox(0,"Buffer realloc() to same size",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
        bf2=freesafebounds(g_mallocstack[i].buf,g_mallocstack[i].len,title);
      }
    } else if (!allownull) MessageBox(0,"Attempt to realloc NULL buf",title, MB_OK|MB_ICONSTOP);
    bf2=reallocsafebounds(bf2,ct);
    if (bf2) {
      int i;
      for(i=0; i<NELEM(g_mallocstack); i++) if (g_mallocstack[i].buf==bf) {g_mallocstack[i].buf=bf2; g_mallocstack[i].len=bf2?ct:0; if (!g_mallocstack[i].title) g_mallocstack[i].title=bf2?title:NULL; goto fnd2;}
      MessageBox(0,"no more space in malloc stack",title, MB_OK|MB_ICONSTOP);
    }
  } else if (bf) freesafe(bf,title);
fnd2:
  return(bf2);
}

#define mallocsafeinit() memset(g_mallocstack,0,sizeof(g_mallocstack))

EXTERNC void mallocsafedone(void) {
  unsigned badct; int i; for(badct=0, i=0; i<NELEM(g_mallocstack); i++) if (g_mallocstack[i].buf) {badct++; MessageBox(0,g_mallocstack[i].title,"mallocsafedone", MB_OK|MB_ICONSTOP); free(freesafebounds(g_mallocstack[i].buf,g_mallocstack[i].len,"mallocsafedone"));}
  if (badct) {
    char debug[256]; snprintf(debug,sizeof(debug),"%u malloc() buffers not free'd\n",badct); MessageBox(0,debug,"mallocsafedone", MB_OK|MB_ICONSTOP);
  }
  mallocsafeinit();
}

#define mallocsafe(ct,ti) reallocsafeX(NULL,ct,ti,1)
#define reallocsafe(bf,ct,ti) reallocsafeX(bf,ct,ti,0)
#define reallocsafeNULL(bf,ct,ti) reallocsafeX(bf,ct,ti,1)
#define malloc malloc_unsafe
#define realloc realloc_unsafe
#define calloc calloc_unsafe /* I never use calloc() but just in case someone tries */
#define free free_unsafe
#define strdup strdup_unsafe

EXTERNC char *strdupsafe(const char *source,char *title) {
  unsigned ls=strlen(source)+1;
  char *rv=(char *)mallocsafe(ls,title);
  if (rv) memcpy(rv,source,ls);
  return(rv);
}

#if 1
#define testmallocsafe()
#else
EXTERNC void testmallocsafe(void) {
  char *test,*test2;
  mallocsafeinit();
  if (test=(char *)mallocsafe(0,"test")) MessageBox(0,"Improper return #1",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  freesafe(test,"test"); // free NULL buf
  if (!(test=(char *)mallocsafe(10,"test"))) MessageBox(0,"Improper return #2",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  memset(test,'@',10);
  if (!(test=(char *)reallocsafe(test,10,"test"))) MessageBox(0,"Improper return #3",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  test[-1]='x'; // buffer beyond bounds
  test2=(char *)reallocsafe(NULL,11,"test2"); // realloc NULL buff
  test2[11]='x';
  freesafe(test,"test"); // no error
  freesafe(test,"test"); // free never allocated
  freesafe(NULL,"test");
  mallocsafedone();
}
#endif

#else
#define mallocsafe(ct,ti) malloc(ct)
#define reallocsafe(bf,ct,ti) realloc(bf,ct)
#define reallocsafeNULL(bf,ct,ti) realloc(bf,ct)
#define freesafe(bf,ti) free(bf)
#define strdupsafe(bf,ti) strdup(bf)
#endif

#if NPPDEBUG
EXTERNC char *memdupsafe(const char *source,unsigned ls,char *title)
#else
#define memdupsafe(bf,ln,ti) memdup(bf,ln)
EXTERNC char *memdup(const char *source,unsigned ls)
#endif
{
  char *rv=(char *)mallocsafe(ls,title);
  if (rv) memcpy(rv,source,ls);
  return(rv);
}

#if NPPDEBUG
EXTERNC char *memdupzsafe(const char *source,unsigned ls,char *title)
#else
#define memdupzsafe(bf,ln,ti) memdupz(bf,ln)
EXTERNC char *memdupz(const char *source,unsigned ls)
#endif
{
  char *rv=(char *)mallocsafe(ls+1,title);
  if (rv) {
    memcpy(rv,source,ls);
    rv[ls]='\0';
  }
  return(rv);
}

EXTERNC size_t roundtonextpower(size_t numo) { /* round to next power */
  size_t pow,num;
  num=numo;
  if (num>=32768) {pow=15; num >>= 15;}
  else if (num>1024) {pow=10; num >>=10;}
  else if (num>128) {pow=5; num>>=5;}
  else pow=0;
  while(num>1) {pow++; num>>=1;}
  num=1<<pow;
  if (num<numo) num<<=1;
  return(num);
}

// returns the number of bytes *dest was changed by so the caller can update their pointers too, including when *dest becomes NULL by accident.
// If you are certain that *dest started non null, you can be sure that it hasn't become NULL if the return value is 0
/* Constant realloc()'s are abusive to memory because realloc() must copy the data to a new malloc()'d area.
   On the other hand, slack space on buffers that will never use it is wasteful.
   Choose strategy to minimize realloc()'s while making good use of memory */
#define ARMSTRATEGY_INCREASE 0 //increase buffer with slack space when necessary; good for a constantly expanding buffer
#define ARMSTRATEGY_MAINTAIN 1 //increase buffer only the minimum amount, buffer will not be reduced if too large; good for a buffer that will be reused with some large and some small allocations
#define ARMSTRATEGY_REDUCE 2   //increase buffer only the minimum amount, reduce buffer if too large; good for buffers of known size or that will only be used once
// If choosing ARMSTRATEGY_REDUCE for a new buffer it is usually better to just use malloc() yourself and not call armrealloc()
// clear=1, memset new space to 0
//size_t g_armrealloc_cutoff=16384; /* sizeof(buf)<=, next size doubled, >, added */
EXTERNC int armrealloc(char **dest,unsigned *destsz,unsigned newsize,int strategy,int clear
#if NPPDEBUG
,char *title
#define armreallocsafe armrealloc
#define THETITLE title
#else
#define armreallocsafe(dt,ds,ns,st,cl,ti) armrealloc(dt,ds,ns,st,cl)
#define THETITLE "armrealloc" /* the macros discards this */
#endif
) {
  unsigned rv=0,oldsz=(*dest)?(*destsz):0;
  char *dest1;
  if (strategy==ARMSTRATEGY_REDUCE && *destsz != newsize) { *destsz = newsize; goto doit;} // without this goto, the conditionals were so convoluted that they never worked
  if (strategy==ARMSTRATEGY_INCREASE){
    if (newsize < 64) newsize=64;
    if (!*dest && newsize < *destsz) newsize=*destsz;
  }
  if (!*dest || *destsz <  newsize) {
    *destsz = (strategy==ARMSTRATEGY_INCREASE)?roundtonextpower(newsize):newsize;
doit:
    dest1=*dest;
    dest1= (dest1)?(char *)reallocsafe(dest1,*destsz,THETITLE):(char *)mallocsafe(*destsz,THETITLE);
      //if (dest1) strcpy((dest1+*destsz-1),"#$"); // $ is beyond string
    rv=dest1-*dest;
    if (dest1 && clear && *destsz>oldsz) memset(dest1+oldsz,0,*destsz-oldsz);
    *dest=dest1;
  }
  return(rv);
}
#undef THETITLE

/* strcpyarm() append-realloc-malloc; This function is designed to
   continually add onto the end of a string. It copies source to *dest+*destlen
   and *destlen += strlen(source). If the new data including \0 would exceed *destsz, the buffer is realloc()'d
   larger and *destsz will be updated with the new size. One of the handy side affects of the arm routines
   is that buffers are no longer created on the stack where they can endanger other stack variables.
   Hackers will find it difficult to overrun a buffer and when they do, all they will trash is the heap.
   returns the number of bytes *dest was changed by during a realloc()
   if source==NULL then no action is taken and a 0 is returned
   *dest must be NULL or a malloc()'d pointer. *dest must not be a stack or data pointer
   if *dest==NULL, a new buffer will be malloc()'d to hold source, *destlen will start at 0
     you can force the new buffer to be larger than source by setting *destsz to the desired starting size
     *destsz should be set to 0 if you do not wish to specify a size for the initial buffer
   if *dest is not NULL, *destsz is always the actual size of the
     malloc()'d buffer so should never be altered while in use unless you free() and reallocate *dest yourself.
   destlen==NULL disables append
     source will always be placed at *dest
     the buffer will always be allocated to it's minimum possible size
     when destlen!=NULL, the buffer will usually have slack space on the end for more things to be added
   dest,destsz cannot be NULL
   On rare occasion, a realloc() will fail and a once valid *dest may be sent back as NULL,
     when this happens *destsz,*destlen will be left at their most recent values though
     they are no longer valid. The next call will start *destlen at 0 and will attempt to allocate *destsz or strlen(source) bytes of memory depending on destlen
   maxlen is the maximum number of characters to copy from source, or (unsigned)-1 to copy all characters. */
// someday we may need a strncpyarmfree() which frees source after it is used
EXTERNC int strncpyarm(char **dest,size_t *destsz,size_t *destlen,const char *source,size_t maxlen
#if NPPDEBUG
,char *title
#define strncpyarmsafe strncpyarm
#define strcpyarmsafe(buf,bufsz,bufl,scsrc,ti) strncpyarm(buf,bufsz,bufl,scsrc,(unsigned)-1,ti)
#define THETITLE title
#else
#define strncpyarmsafe(dt,ds,dl,st,ml,ti) strncpyarm(dt,ds,dl,st,ml)
#define strcpyarmsafe(buf,bufsz,bufl,scsrc,ti) strncpyarm(buf,bufsz,bufl,scsrc,(unsigned)-1)
#define THETITLE "strncpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1,slen;
  int rv=0;
  if (source) { // maxlen==0 is perfectly valid
    slen=strlen(source);
    if (slen>maxlen) slen=maxlen;
    destlen1=(*dest && destlen)?*destlen:0;
    if (*dest && !destlen1 && *destsz!=slen+1) {
      freesafe(*dest,THETITLE);
      *dest=NULL;
      *destsz=slen+1;
    }
    rv=armreallocsafe(dest,destsz,destlen1+slen+1,destlen?ARMSTRATEGY_INCREASE:ARMSTRATEGY_REDUCE,0,THETITLE);
    if (*dest) {
      if (slen) {
        memcpy(*dest+destlen1,source,slen);
        destlen1 += slen;
      }
      *(*dest+destlen1)='\0';
      if (destlen) *destlen=destlen1;
    }
  }
  return(rv);
}
#undef THETITLE

EXTERNC int memcpyarm(char **dest,size_t *destsz,size_t *destlen,const char *source,size_t slen
#if NPPDEBUG
,char *title
#define memcpyarmsafe memcpyarm
#define THETITLE title
#else
#define memcpyarmsafe(dt,ds,dl,st,ml,ti) memcpyarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1;
  int rv=0;
  if (source) { // slen=0 must at least allocate memory
    destlen1=(*dest && destlen)?*destlen:0;
    if (*dest && !destlen1 && *destsz!=slen) {
      freesafe(*dest,THETITLE);
      *dest=NULL;
      *destsz=slen;
    }
    rv=armreallocsafe(dest,destsz,destlen1+slen,destlen?ARMSTRATEGY_INCREASE:ARMSTRATEGY_REDUCE,0,THETITLE);
    if (*dest) {
      if (slen) {
        memcpy(*dest+destlen1,source,slen);
        destlen1 += slen;
      }
      if (destlen) *destlen=destlen1;
    }
  }
  return(rv);
}
#undef THETITLE

EXTERNC int memsetarm(char **dest,size_t *destsz,size_t *destlen,int chr,size_t slen
#if NPPDEBUG
,char *title
#define memsetarmsafe memsetarm
#define THETITLE title
#else
#define memsetarmsafe(dt,ds,dl,st,ml,ti) memsetarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
) {
  size_t destlen1;
  int rv=0;
    destlen1=(*dest && destlen)?*destlen:0;
    if (*dest && !destlen1 && *destsz!=slen) {
      freesafe(*dest,THETITLE);
      *dest=NULL;
      *destsz=slen;
    }
    rv=armreallocsafe(dest,destsz,destlen1+slen,destlen?ARMSTRATEGY_INCREASE:ARMSTRATEGY_REDUCE,0,THETITLE);
    if (*dest) {
      if (slen) {
        memset(*dest+destlen1,chr,slen);
        destlen1 += slen;
      }
      if (destlen) *destlen=destlen1;
    }
  return(rv);
}
#undef THETITLE

/* snprintfX() attempts to perfectly duplicate what sprintf() does
   without the unsafeness or undesirable behaviour of sprintf()
   or the bugs and problems of various snprintf()'s
   Call: snprintfX(buf,sizeof(buf),"format string",...);
   Returns: number of non null characters written.
   Unlike the official snprintf():
     * snprintfX() writes no more than sizeof(buf)-1 non null characters plus a guaranteed \0 ending.
     * snprintfX() always returns the exact number of non null characters written
     * snprintfX() does not return a -1 if an error occurs or more buffer space is needed
     * snprintfX() does not return the total number bytes that would be
         needed to store the entire string when there is not enough space
   If you need any of that functionality, you must call the real functions and deal with the bugs as vsarmprintf() does */
EXTERNC size_t snprintfX(char *buffer,size_t buffersz,const char *format,...) {
  size_t rv=0;
  va_list ap;
  if (buffer && buffersz) {
    va_start(ap, format);
    buffer[0]='\0';
    rv=vsnprintf(buffer,buffersz,format,ap);
    buffer[buffersz-1]='\0';
    if (rv==(unsigned)-1) rv=strlen(buffer);
    if (rv>=buffersz) rv=buffersz-1;
    va_end(ap);
  }
  return(rv);
}

/* sprintf() using the same buffer system as strncpyarm()
   see strncpyarm() for parameter meanings
   http://libslack.org/manpages/snprintf.3.html
   smprintf() handles the broken vsnprintf() implementations that return -1 when there isn't enough space
     or ones that ensure or don't ensure a \0 at the end
   The v...() routines are rarely used directly. They are made so the non v routine wrappers and custom wrappers can be built.*/
EXTERNC size_t vsarmprintf(char **dest,size_t *destsz,size_t *destlen,const char *format,va_list ap2) {
  size_t vs=0,destlen1,destsz1=*destsz;
  char *dest1=*dest;
  va_list ap;

  destlen1=destlen?*destlen:0;
  if (!dest1) {
    destlen1=0;
    if (destsz1<8) destsz1=8;
    dest1=(char *)mallocsafe(destsz1,"vsarmprintf");
  }
  if (dest1 && destsz1<8+destlen1) { //guarantee at least 8 characters into which vsnprintf() can write
    destsz1=8+destlen1;
    dest1=(char *)reallocsafe(dest1,destsz1,"vsarmprintf");
  }
  goto bottest; while(dest1 && destsz1 && !(dest1[destsz1-1]='\0') && ((vs=vsnprintf(dest1+destlen1,destsz1-destlen1,format,ap))>(destsz1-destlen1) || dest1[destsz1-1])) {
    if (vs == (unsigned)-1) destsz1=roundtonextpower(destsz1*2);
    else destsz1=destlen?roundtonextpower(destlen1+vs+1):destlen1+vs+1;
#if NPPWINDOWS /* { no recursive calls please */
    //char debug[256]; snprintfX(debug,sizeof(debug),"vsnprintf() returns %u; Trying length %u",vs,destsz1); MessageBox(g_nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
    if (!destlen1) {
      freesafe(dest1,"vsarmprintf");
      dest1=(char *)mallocsafe(destsz1,"vsarmprintf");
    } else dest1=(char *)reallocsafe(dest1,destsz1,"vsarmprintf");
bottest:
#ifdef __WATCOMC__
    memcpy(&ap[0],&ap2[0],sizeof(ap[0])); /* It appears that vsnprintf() increments ap[0] which we don't want. What a buggy compiler! */
#else
    ap=ap2;
#endif
    //memset(dest1+destlen1,'@',destsz1-destlen1); dest1[destsz1]='?'; // beyond string
  }
  if (dest1) {
#if NPPDEBUG /* { */
    if (vs != strlen(dest1+destlen1)) { /* no recursive calls please */
      char debug[256]; snprintfX(debug,sizeof(debug),"vs=%u strlen(dest1)=%u",vs,strlen(dest1)); MessageBox(0,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
      vs = strlen(dest1+destlen1);
    }
    if (destsz1<=vs+destlen1) {
      char debug[256]; snprintfX(debug,sizeof(debug),"Buffer too short, destlen1=%u vs=%u",destsz1,vs); MessageBox(0,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
    }
#endif /* } */
    if (!destlen && destsz1>vs+1+destlen1) {
      destsz1=vs+1+destlen1;
      dest1=(char *)reallocsafe(dest1,destsz1,"vsarmprintf");
#if NPPWINDOWS /* { no recursive calls please */
      //char debug[256]; snprintfX(debug,sizeof(debug),"Final length (including \\0) was %u",vs+1); MessageBox(g_nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
    }
  }
  if (!(*dest=dest1)) vs=0;
  *destsz=destsz1;
  if (destlen) *destlen=destlen1+vs;
  return(vs);
}

EXTERNC size_t sarmprintf(char **dest,size_t *destsz,size_t *destlen,const char *format,...) {
  size_t rv;
  va_list ap;
  va_start(ap,format);
  rv=vsarmprintf(dest,destsz,destlen,format,ap);
  va_end(ap);
  return(rv);
}

/* snprintf() falls short of the mark.
   What we really need is smprintf(...) AKA mprintf(...).
   Like strdup() it malloc()'s sufficient space and places the sprintf()
   results in it. The caller will get the entire result without needing to
   overguess and waste space. The caller will free the pointer. The caller
   may create a special routine that uses and free's the pointer all at once. */
EXTERNC char *smprintf(const char *format,...) {
  char *rv=NULL;
#if NPPDEBUG
  size_t rvsz=0;
#else
  size_t rvsz=256;
#endif
  va_list ap;
  va_start(ap,format);
  vsarmprintf(&rv,&rvsz,NULL,format,ap);
  va_end(ap);
  return(rv);
}

/* vsmprintfpath() gets much reduced functionality from smprintf()
   but it gains a couple of features needed for path construction
   and it probably runs a lot faster than the official smprintf()
   so it's better for simple format strings. Simple format strings
   without any of the special functions are interchangable with
   sprintf() variants.
   If you need full sprintf() functionality, use smprintf() or snprintfX() above. */
/* Control characters:
   Supported: exactly as shown; no modifiers allowed!
     %%=%
     %c=character
     %s=string
     %d,%i=integer
     %o=octal
     %u=unsigned
     %x,%X hex
   New and Improved Functionality:
//new feature: %bn# print number in any base, n can be *
//new feature: optional characters also cancel next character if the same
//new feature: %S a string that needs to be free()'d after it is used
//new feature: %s prints (null) when NULL is passed in
//new feature: %#n(...) repeat ... n times
//new feature: %D hex dump of string
     %?_, optional character _ immediately after ? is only inserted if the previous character is not a _
     %C, optional character next in va_args is only inserted if the previous character is not the same
       special note: if the character is a space then it is only inserted if the previous character is not whitespace
     %#n* ; %#n#_ where n is a number or * and _ is a character;
       inserts n characters _ of your choice: example: %#57#? inserts 57 question marks. n may be a * to extract the number from the arglist
       if the second # is a * then the character is drawn from the arglist
       %#** (both from arglist) or %#57* (char from arglist)
   Not Supported:
     Any internal modifiers
     float types
   Example: smprintfpath("%c%?:%s%?\\%s%?\\%s",'C',"\\WINDOWS\","TEMP","TEMPFILE.TXT")
     -> C:\WINDOWS\TEMP\TEMPFILE.TXT
*/
EXTERNC size_t vsarmprintfpath(char **dest,unsigned *destsz,unsigned *destlen,const char *format,va_list ap2) {
  size_t tlen=0,llen;
  char *cx;
  char *argst,*dest1,lastch,checkch,cmd[3],tempspace[128];
  unsigned pass,argu;
  size_t destlen1=(*dest && destlen)?*destlen:0;
  for(pass=0; pass<2; pass++) {
    va_list ap;
#ifdef __WATCOMC__
    memcpy(&ap[0],&ap2[0],sizeof(ap[0]));
#else
     ap=ap2;
#endif
    dest1=*dest+destlen1; // not used on the first pass, prevents compiler warnings
    for(lastch='\0',cx=(char *)format; *cx; cx++) {
      if (*cx!='%') {
pctch:
        lastch=*cx;
        goto dochar;
      } else switch(*++cx) {
        // better idea: insert the % symbol and go on to the next character
      case '\0': cx--; break; /* feed the \0 back to kill the for loop */
      case '%': lastch  = '%'; goto dochar;
      case 'c': lastch  = (int)va_arg(ap,unsigned); goto dochar;
      case 'C': checkch = (int)va_arg(ap,unsigned); goto checkchar;
      case '?':
        cx++;
        checkch=*cx;
checkchar:
        if (!(checkch==' ' && isspace(lastch) || lastch==checkch)) {
          lastch=checkch;
dochar:   if (!pass) {
            tlen++;
          } else {
            *dest1=lastch;
            dest1++;
            *dest1='\0';
          }
        }
        break;
      case 'd':
      case 'i':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
        argu = va_arg(ap,unsigned);
        memcpy(cmd,"%c",3);
        cmd[1]=*cx;
        snprintfX(tempspace,sizeof(tempspace),cmd,argu);
        argst=tempspace;
        goto dostring;  /* this saves code but wastes a strlen on a known short string */
      case 's':
        argst = va_arg(ap,char *);
dostring:
        if (!pass) {
          if (llen=strlen(argst)) {
            lastch=argst[llen-1];
            tlen+=llen;
          }
        } else {
          while(*dest1=*argst) {dest1++; argst++;} // stpcpy() isn't always available
          //memcpy(dest,argst,strlen(argst)+1);  %S support
          if (dest1>*dest) lastch=*(dest1-1);
        }
        break;
      case '#':
        cx++;
        if (!*cx) continue;
        else if (*cx=='*') {
          llen=va_arg(ap,unsigned);
          cx++;
          if (!*cx) continue;
        } else llen=strtol(cx,&cx,0);
        if (llen) {
          if (*cx=='*') {
            lastch=va_arg(ap,int);
          } else {
            cx++;
            lastch=*cx;
          }
          if (!pass) {
            tlen+=llen;
          } else {
            memset(dest1,lastch,llen);
            dest1 += llen;
            *dest1='\0';
          }
        } else cx++;
        break;
default: goto pctch;
      }
    }
    if (!pass) {
      armreallocsafe(dest,destsz,destlen1+tlen+1,destlen?ARMSTRATEGY_INCREASE:ARMSTRATEGY_REDUCE,0,"vsarmprintfpath");
      if (!*dest) break;
    }
  }
  if (destlen) *destlen += tlen;
  return(tlen);
}

EXTERNC size_t sarmprintfpath(char **dest,size_t *destsz,size_t *destlen,const char *format,...) {
  size_t rv;
  va_list ap;
  va_start(ap,format);
  rv=vsarmprintfpath(dest,destsz,destlen,format,ap);
  va_end(ap);
  return(rv);
}

EXTERNC char *smprintfpath(const char *format,...) {
  char *rv=NULL;
#if NPPDEBUG
  size_t rvsz=0;
#else
  size_t rvsz=256;
#endif
  va_list ap;
  va_start(ap,format);
  vsarmprintfpath(&rv,&rvsz,NULL,format,ap);
  va_end(ap);
  return(rv);
}

/* Just like MessageBox() except that lpText must be NULL or a malloc()'d pointer that will be free()'d after display.
   This allows you to display message text of unknown size without creating and freeing a pointer.
   If lpText is NULL meaning the malloc() failed, a nondescript default error message is printed instead.
   Ideally lpText will be the result of a strdup(), smprintf(), smprintfpath(), or other malloc() function.
   Example: MessageBoxFree(hWnd,smprintf("Message of unknown length:\r\n%s",str),"Caption",MB_OK); */
EXTERNC int MessageBoxFree(HWND hWnd,TCHAR *lpText,LPCTSTR lpCaption,UINT uType) {
  int rv=MessageBox(hWnd,lpText?lpText:"a memory problem occured",lpCaption,uType);
  if (lpText) freesafe(lpText,"MessageBoxFree");
  return(rv);
}

#if NPPDEBUG
EXTERNC void testfree(int tofree,char *res,char *realstring,size_t rv) {
  if (res) {
    if (strcmp(res,realstring)) MessageBox(0,res,"String Mismatch", MB_OK|MB_ICONSTOP);
    if (tofree) freesafe(res,"testfree");
  } else MessageBox(0,"malloc() failed",PLUGIN_NAME, MB_OK|MB_ICONSTOP);
  if (rv != (unsigned)-1 && rv != strlen(realstring)) MessageBoxFree(0,smprintf("Mismatch: rv=%u strlen()=%u",rv,strlen(realstring)),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
}

EXTERNC void testsprintfuncs(void) {
  //char *res;
  size_t rv;
  char buf[8];
  char *zbuf,*zbuf2; unsigned zbufsz,zbufsz2,temp,strategy,domalloc,want; int dx;

  for(temp=4; temp<=4096; temp<<=1) for(dx=-1; dx!=2; dx++) for(strategy=0; strategy<=2; strategy++) for(domalloc=0; domalloc<=1; domalloc++) {
    zbufsz=temp+dx;
    zbuf=domalloc?(char *)mallocsafe(zbufsz,"testsprintfuncs"):NULL;
    armreallocsafe(&zbuf,&zbufsz,temp+dx,strategy,0,"testsprintfuncs");
    want=strategy==ARMSTRATEGY_INCREASE?roundtonextpower(temp+dx):temp+dx;
    if (domalloc) want=temp+dx;
    if (strategy==ARMSTRATEGY_INCREASE && want<64) want=64;
    if (zbufsz != want) MessageBoxFree(0,smprintf("Mismatch: strategy:%u temp+dx=%u want=%u zbufsz=%u zbufwas=%s",strategy,temp+dx,want,zbufsz,domalloc?"...":"NULL"),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
    freesafe(zbuf,"testsprintfuncs");
  }

  for(temp=1; temp<130; temp++) {
    zbuf=NULL; zbufsz=0;
    sarmprintf(&zbuf,&zbufsz,NULL,"%*s",temp,"X");
    zbuf2=NULL;zbufsz2=0;
    strcpyarmsafe(&zbuf2,&zbufsz2,NULL,zbuf,"testsprintfuncs");
    freesafe(zbuf2,"testsprintfuncs");
    freesafe(zbuf,"testsprintfuncs");
    //zbuf=NULL; zbufsz=0;
    zbuf=NULL; zbufsz=0;
    sarmprintf(&zbuf,&zbufsz,NULL,"%*s",131-temp,"X");
    zbuf2=NULL;zbufsz2=0;
    strcpyarmsafe(&zbuf2,&zbufsz2,NULL,zbuf,"testsprintfuncs");
    freesafe(zbuf2,"testsprintfuncs");
    freesafe(zbuf,"testsprintfuncs");
  }

  //test with smprintf() initial buffer size of 8
  testfree(1,smprintf("Howdy %d partner",57),"Howdy 57 partner",(unsigned)-1); // too long
  testfree(1,smprintf("123456"),"123456",(unsigned)-1); // too short
  testfree(1,smprintf("x2%s","ex"),"x2ex",(unsigned)-1); // too short
  testfree(1,smprintf("1234567"),"1234567",(unsigned)-1); // just right
  testfree(1,smprintfpath("%s%?\\%s%?\\%s","A","B\\","C"),"A\\B\\C",(unsigned)-1);
  rv=snprintfX(buf,sizeof(buf),"123456789ABCDEF");
  testfree(0,buf,"1234567",rv);
  rv=snprintfX(buf,sizeof(buf),"123");
  testfree(0,buf,"123",rv);
}
#endif

// When enabled, this will ensure that string lengths are being maintained properly by the highly detailed
// string transforms. When they are known to be working, turning off NPPDEBUG will disable all this
// slow code.
#if NPPDEBUG && 1 /* { binary safe functions no longer permit this testing */
EXTERNC void *memmovetest(void *dest,void *src,size_t n) {
  size_t m;
  m=strlen((const char *)src);
  if (m+1 != n) {
#if NPPWINDOWS /* { */
      MessageBoxFree(g_nppData._nppHandle,smprintf("memmovetest wrong SLN strlen(+1):%u != SLN:%u",strlen((const char *)src)+1,n),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#else /* } else { */ /* Borland C for DOS */
     printf("Wrong SLN:%u != strlen(+1):%d\n",n,m+1);
#endif /* } */
     n=m;
  }
  return(memmove(dest,src,n));
}
#else /* } else { */
#define memmovetest memmove
#endif /* } */

// unlike other arm routines, destlen is required here because memmovearm() is not interested in appending anything.
// returns the number of bytes *dest was changed by during a realloc() so the caller can adjust their pointers
// memmovearm always moves one extra byte assuming the caller is trying to maintain a \0 terminated C-string
EXTERNC int memmovearm(char **dest,unsigned *destsz,unsigned *destlen,char *destp,char *sourcep
#if NPPDEBUG
,int notest
#endif
){  unsigned rv=0;
  if (destp != sourcep) {
    if (rv=armreallocsafe(dest,destsz,*destlen+1+destp-sourcep,ARMSTRATEGY_INCREASE,0,"memmovearm")) {
      destp += rv;
      sourcep += rv;
    }
    if (*dest) {
      //memset(*dest+*destlen,'@',*destsz-*destlen); *(*dest+*destlen)='%'; // % is \0
#if NPPDEBUG
      if (notest) memmove(destp,sourcep,(*destlen)-(sourcep-*dest)+1); else
#endif
      memmovetest(destp,sourcep,(*destlen)-(sourcep-*dest)+1);
      *destlen += destp-sourcep;
    }
  }
  return(rv);
}
#if NPPDEBUG
#define memmovearmtest memmovearm
#else
#define memmovearmtest(dt,ds,dl,dp,sp,nt) memmovearm(dt,ds,dl,dp,sp)
#endif

// because so many c string functions must call strlen() before operating, these mem...
// functions are more efficient on huge buffers. If you wish to provide a \0 terminated C-string
// calculate the strlen() yourself as most c string functions do every time they
// are called.
EXTERNC void memcqspnstart(const char *find,unsigned findl,unsigned *quick) {
  unsigned q;
  for(q=0; q<256/(sizeof(unsigned)*8); q++) quick[q]=0;
  while(findl) {
    quick[(unsigned)*(unsigned char *)find/(sizeof(unsigned)*8)] |= 1<<(unsigned)*(unsigned char *)find%(sizeof(unsigned)*8);
    find++;
    findl--;
  }
}

// strcqspn quick version for large find strings
// end=buf+buflen
// for C strings this is the position of the \0
// for buffers, this is 1 character beyond the end (same place)
// returns end when the search fails
EXTERNC char *memcqspn(char *buf,char *end,unsigned *quick) {
  for(; buf<end; buf++) {
    if (quick[(unsigned)*(unsigned char *)buf/(sizeof(unsigned)*8)] & 1<<((unsigned)*(unsigned char *)buf%(sizeof(unsigned)*8))) return(buf);
  }
  return(end);
}

EXTERNC char *memqspn(char *buf,char *end,unsigned *quick) {
  for(; buf<end; buf++) {
    if (!(quick[(unsigned)*(unsigned char *)buf/(sizeof(unsigned)*8)] & 1<<((unsigned)*(unsigned char *)buf%(sizeof(unsigned)*8)))) return(buf);
  }
  return(end);
}

// strcspn: returns the first location in buf of any character in find
// extra performance derived from pam_mysql.c (GPL)
EXTERNC char *memcspn(char *ibuf,char *iend,const char *ifind,unsigned findl) {
  const unsigned char *find=ifind;
  unsigned char *buf=ibuf,*end=iend;
  switch(findl) {
  case 0: break;
  case 1: if (buf<end) {
      char *rv;
      if (rv=(char *)memchr(buf,*find,end-buf)) return(rv);
    } break;
  case 2: {
      unsigned char c1=find[0],c2=find[1];
      for(; buf<end; buf++) if (*buf==c1 || *buf==c2) return(buf);
    } break;
  default: if (buf<end) {
      const unsigned char *findt,*finde=find+findl;
      unsigned char min=(unsigned)-1,max=0,and_mask=~0,or_mask=0;
      for(findt=find; findt<finde; findt++) {
        if (max<*findt) max=*findt;
        if (min>*findt) min=*findt;
        and_mask &= *findt;
        or_mask |= *findt;
      }
      for(; buf<end; buf++) {
        if (*buf>=min && *buf<=max && (*buf & and_mask) == and_mask && (*buf | or_mask) && memchr(find,*buf,findl)) return(buf);
      }
    } break;
  }
  return(end);
}

// strspn: returns the first location in buf of any character not in find
// extra performance derived from pam_mysql.c (GPL)
EXTERNC char *memspn(char *ibuf,char *iend,const char *ifind,unsigned findl) {
  unsigned char *buf=ibuf,*end=iend;
  const unsigned char *find=ifind;
  switch(findl) {
  case 0: return(buf);
  case 1: {
      unsigned char c1=find[0];
      while(buf<end && *buf==c1) buf++; // if I had time I'd implement this as REPE SCASB
    }
    return(buf);
  case 2: {
      unsigned char c1=find[0],c2=find[1];
      while(buf<end && *buf==c1 || *buf==c2) buf++;
    }
    return(buf);
  default: if (buf<end) {
      const unsigned char *findt,*finde=find+findl;
      unsigned char min=(unsigned)-1,max=0,and_mask=~0,or_mask=0;
      for(findt=find; findt<finde; findt++) {
        if (max<*findt) max=*findt;
        if (min>*findt) min=*findt;
        and_mask &= *findt;
        or_mask |= *findt;
      }
      for(; buf<end; buf++) {
        if (*buf<min || *buf>max || (*buf & and_mask) != and_mask || !(*buf | or_mask) || !memchr(find,*buf,findl)) return(buf);
      }
    } break;
  }
  return(end);
}

// if buf>end then the search goes backwards
EXTERNC char *memchrX(char *buf,char *end,char find) {
  char *rv=NULL;
  if (end<buf) { // someone ambitious could implement the REPNE SCASB i86 instructino
    for(; buf>end; buf--) if (*buf==find) {rv=buf; break;}
  } else if (buf<end) rv=(char *)memchr(buf,find,end-buf);
  return rv?rv:end;
}

// when reverse string searching gets here, use memchrX above
EXTERNC char *memstr(char *buf,char *end,const char *find,unsigned findl) {
  if (findl>1) {
    if (buf<=end-findl) {
      while((buf=(char *)memchr(buf,*find,end-buf)) && buf<=end-findl) {
        if (!memcmp(buf+1,find+1,findl-1)) return(buf);
        buf++;
      }
    }
  } else if (findl==1 && buf<end) {
    char *rv;
    if (rv=(char *)memchr(buf,*find,end-buf)) return(rv);
  }
  return(end);
}

// copies a buf:len to a limited size szString buffer, sSource is not checked for zero termination
EXTERNC char *strncpymem(char *szDest,size_t uDestSz,char *sSource,unsigned uSourceLen) {
  if (szDest && uDestSz) {
    if (sSource) {
      if (uSourceLen>=uDestSz) uSourceLen=uDestSz-1;
      if (uSourceLen) memcpy(szDest,sSource,uSourceLen);
    } else uSourceLen=0;
    szDest[uSourceLen]='\0';
    return szDest+uSourceLen;
  }
  return NULL;
}

/* strtok() is defective in some compilers and the bad calling sequence makes it difficult to be thread safe.
   We'll redo it and fix all of it's unreliability problems.
   szStr always points at the beginning
   Start uPos at 0 and pass the modified value back each time you want a new string extracted
   uPos is maintained separately so the original string can be optimized in the caller's code
   *uPos can be NULL if you only need a single decoding */
char *strtokX(char *szStr,unsigned *uPos,const char *szDeli) {
  char *rve,*rv=szStr;
  if (uPos) rv+= *uPos;
  if (*rv) {
    rv += strspn(rv,szDeli);
    rve = rv+strcspn(rv,szDeli);
    if (*rve) *rve++='\0';
    if (uPos) *uPos = rve-szStr;
  }
  return rv;
}

//http://www.catch22.net/source/snippets.asp
#define ISPOW2(x)  (x && !(x & (x-1)))

// check to see if a number is a power of two
// returns the power+1 such that 1<<(powcheck(num)-1)=num
// returns 0 if the num is not a power of 2, such as when num==0
EXTERNC unsigned powcheck(unsigned num) {
  unsigned rv=0;

  if (ISPOW2(num)) for(rv=1; num>1; num>>=1,rv++);
  return(rv);
}

// SUBSECTION Sort routines
/* I discovered the need for a safer sort than qsort which can blow up the
 * stack with cleverly designed input. I also discovered the need for a
 * stable sort. Merge was chosen for the stable sort and Introsort was
 * chosen for the stack safe qsort. A special version for width==sizeof(int)
 * is provided to better optimize the most common record width. Should
 * anything go wrong, the routines bail to the c-lib qsort(). All the
 * sort routines are designed to be call compatible with qsort(). */
#if 1 && NPPDEBUG
#define movect(num) g_uMoves+=(num);
#define setmoves(num) g_uMoves=(num);
#define getmoves() (g_uMoves)
  unsigned long g_uMoves;
#else
#define movect(num)
#define setmoves(num)
#define getmoves() (0LU)
#endif
#define INTROSORTENABLE 1 /* set to 1 for introsort, 0 for original qsort */
#define INTROSTACKLEVEL 16 /* The stack depth permitted before switching to heapsort */
// http://linux.wku.edu/~lamonml/kb.html (GNU GPL version 2)
EXTERNC void _mergesortint(unsigned *pbase,unsigned *ptempo,size_t left,size_t mid,size_t right,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
#define width 1
  unsigned *pleft,*plefte,*pmid,*pright,*ptemp;

  pleft=pbase+left*width;
  pmid=pbase+mid*width;
  plefte=pmid-width;
  ptemp=ptempo;
  pright=pbase+right*width;

  if (pleft <= plefte && pmid <= pright) while(1) {
    if (fcmp(pleft,pmid)<=0) {
      *ptemp=*pleft;
      movect(1)
      ptemp+=width;
      pleft+=width;
      if (pleft>plefte) break;
    } else {
      *ptemp=*pmid;
      movect(1)
      ptemp+=width;
      pmid+=width;
      if (pmid>pright) break;
    }
  }
  while(pleft<=plefte) {
      *ptemp=*pleft;
      movect(1)
      ptemp+=width;
      pleft+=width;
  }
  while(pmid<=pright) {
      *ptemp=*pmid;
      movect(1)
      ptemp+=width;
      pmid+=width;
  }
  memcpy(pbase+left*width,ptempo,(char *)ptemp-(char *)ptempo);
  movect(ptemp-ptempo);
#undef width
}

// this verion isn't much less efficient. The only multiplication is at the beginning
EXTERNC void _mergesort(void *base,void *temp,size_t left,size_t mid,size_t right,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  char *pleft,*plefte,*pmid,*pright,*ptemp;

  pleft=(char *)base+left*width;
  pmid=(char *)base+mid*width;
  plefte=pmid-width;
  ptemp=(char *)temp;
  pright=(char *)base+right*width;

  if (pleft <= plefte && pmid <= pright) while(1) {
    if (fcmp((void *)pleft,(void *)pmid)<=0) {
      memcpy(ptemp,pleft,width);
      movect(1)
      ptemp+=width;
      pleft+=width;
      if (pleft>plefte) break;
    } else {
      memcpy(ptemp,pmid,width);
      movect(1)
      ptemp+=width;
      pmid+=width;
      if (pmid>pright) break;
    }
  }
  while(pleft<=plefte) {
      memcpy(ptemp,pleft,width);
      movect(1)
      ptemp+=width;
      pleft+=width;
  }
  while(pmid<=pright) {
      memcpy(ptemp,pmid,width);
      movect(1)
      ptemp+=width;
      pmid+=width;
  }
  memcpy((char *)base+left*width,temp,(char *)ptemp-(char *)temp);
  movect(((char *)ptemp-(char *)temp)/width)
}

EXTERNC void _mergesortsplitint(unsigned *base,unsigned *temp,size_t left,size_t right,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned mid;

  if (right > left)   {
    mid = (right + left) / 2;
    _mergesortsplitint(base,temp,left ,mid        ,fcmp);
    _mergesortsplitint(base,temp      ,mid+1,right,fcmp);
    _mergesortint(base,temp,left      ,mid+1,right,fcmp);
  }
}

EXTERNC void _mergesortsplit(void *base,void *temp,size_t left,size_t right,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned mid;

  if (right > left)   {
    mid = (right + left) / 2;
    _mergesortsplit(base,temp,left ,mid        ,width,fcmp);
    _mergesortsplit(base,temp      ,mid+1,right,width,fcmp);
    _mergesort(base,temp,left      ,mid+1,right,width,fcmp);
  }
}

EXTERNC void qsortmerge(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  void *temp;
  if (nelem>1 && width>=1 && (temp=mallocsafe(nelem*width,"msort"))) {
    if (1 && width==sizeof(int)) _mergesortsplitint((unsigned *)base,(unsigned *)temp,0,nelem-1,fcmp);
    else _mergesortsplit(base,temp,0,nelem-1,width,fcmp);
    freesafe(temp,"msort");
  }
}

// http://en.wikipedia.org/wiki/Heapsort
// This unified version is much nicer than the one with the sift() function
EXTERNC void _heapsortint(unsigned *base,unsigned *pivottemp,size_t nelem,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned int n = nelem, i = n/2, parent, child;

  while(1) {
    if (i) {
      i--;
      *pivottemp = base[i];
    } else {
      n--;
      if (n == 0) return;
      *pivottemp = base[n];
      base[n] = base[0];
    }
    movect(1)

    parent = i;
    child = parent*2 + 1;

    while(child < n) {
      if (child+1<n && fcmp(&base[child + 1],&base[child])>0) {
        child++;
      }
      if (fcmp(&base[child],pivottemp)<=0) break;
        base[parent] = base[child];
        movect(1)
        parent = child;
        child = parent*2 + 1;
    }
    base[parent] = *pivottemp;
    movect(1)
  }
}

EXTERNC void _heapsort(void *base,void *pivottemp,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned int n = nelem, i = n/2, parent, child;

  while(1) {
    if (i) {
      i--;
      memcpy(pivottemp,(char *)base+i*width,width); //*pivottemp = base[i];
    } else {
      n--;
      if (n == 0) return;
      memcpy(pivottemp,(char *)base+n*width,width); //*pivottemp = base[n];
      memcpy((char *)base+n*width,base,width); //base[n] = base[0];
    }
    movect(1)

    parent = i;
    child = parent*2 + 1;

    while(child < n) {
      if (child+1<n && fcmp((char *)base+(child+1)*width,(char *)base+child*width)>0) {
        child++;
      }
      if (fcmp((char *)base+child*width,pivottemp)<=0) break;
        memcpy((char *)base+parent*width,(char *)base+child*width,width);
        movect(1)
        parent = child;
        child = parent*2 + 1;
    }
    memcpy((char *)base+parent*width,pivottemp,width); // base[parent] = *pivottemp;
    movect(1)
  }
}

EXTERNC void qsortheap(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  if (nelem>1 && width>=1) {
    if (1 && width==sizeof(int)) {
      unsigned temp;
      _heapsortint((unsigned *)base,&temp,nelem,fcmp);
    } else {
      void *temp;
      if (temp=mallocsafe(width,"qsortheap")) {
        _heapsort(base,temp,nelem,width,fcmp);
        freesafe(temp,"qsortheap");
      } else qsort(base,nelem,width,fcmp);
    }
  }
}

// http://linux.wku.edu/~lamonml/kb.html (GNU GPL version 2)
EXTERNC void _introsortint(unsigned depth,unsigned *base,unsigned *pivottemp,unsigned nelem,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  unsigned *leftpiv=base,*right=base+(--nelem),prn;

  // median of 3: we want the center value in left, the highest value in right, and the lowest value in the center
  // http://www.java2s.com/ExampleCode/Collections-Data-Structure/Quicksortwithmedianofthreepartitioning.htm
  if (nelem>=16) { // less than 16 produces too many extra compares
    unsigned *center=base+nelem/2;
    if      (fcmp(center ,right )>0) {*pivottemp=*right  ;*right  =*center;*center=*pivottemp;movect(3)}
    if      (fcmp(leftpiv,right )>0) {*pivottemp=*leftpiv;*leftpiv=*right ; *right=*pivottemp;movect(3)}
    else if (fcmp(leftpiv,center)<0) {*pivottemp=*leftpiv;*leftpiv=*center;*center=*pivottemp;movect(3)}
    //if (*leftpiv<*center || *leftpiv>*right) printf("F"); // failed to get the lowest value
  }
  *pivottemp = *leftpiv;
  movect(1)
  while(leftpiv<right) {
    if (1) while (1) {
      if (fcmp(right,pivottemp)>=0) {
        if (leftpiv >= --right) break;
      } else {
        *leftpiv = *right;
        movect(1)
        leftpiv++;
        break;
      }
    }
    if (leftpiv<right) while (1) {
      if (fcmp(leftpiv,pivottemp)<=0) {
        if (++leftpiv >= right) break;
      } else {
        *right = *leftpiv;
        movect(1)
        right--;
        break;
      }
    }
  }
  *leftpiv = *pivottemp;
  movect(1)
  prn=leftpiv-base;
  if (prn    > 1) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // if the stack has grown too large, pass off to heap sort
      _heapsortint(base,pivottemp,prn ,fcmp);
    else
      _introsortint(depth+1,base,pivottemp,prn ,fcmp);
  }
  if (nelem > prn+1  ) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // all evidence shows that there is no minimum value where heapsort performs better
      _heapsortint (base+prn+1,pivottemp,nelem-prn,fcmp);
    else
      _introsortint(depth+1,base+prn+1,pivottemp,nelem-prn,fcmp);
  }
}

EXTERNC void _introsort(unsigned depth,void *base,void *pivottemp,unsigned nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  char *leftpiv=(char *)base,*right=(char *)base+(--nelem)*width;
  unsigned prn;

  // median of 3: we want the center value in left, the highest value in right, and the lowest value in the center
  // http://www.java2s.com/ExampleCode/Collections-Data-Structure/Quicksortwithmedianofthreepartitioning.htm
  if (nelem>=16) { // less than 16 produces too many extra compares
    char *center=(char *)base+(nelem/2)*width;
    if      (fcmp(center ,right )>0) {memcpy(pivottemp,right,width)  ;memcpy(right,center,width);memcpy(center,pivottemp,width);movect(3)}
    if      (fcmp(leftpiv,right )>0) {memcpy(pivottemp,leftpiv,width);memcpy(leftpiv,right,width);memcpy(right,pivottemp,width);movect(3)}
    else if (fcmp(leftpiv,center)<0) {memcpy(pivottemp,leftpiv,width);memcpy(leftpiv,center,width);memcpy(center,pivottemp,width);movect(3)}
    //if (*leftpiv<*center || *leftpiv>*right) printf("F"); // failed to get the lowest value
  }
  memcpy(pivottemp,leftpiv,width);
  movect(1)
  while(leftpiv<right) {
    if (1) while (1) {
      if (fcmp(right,pivottemp)>=0) {
        right -= width;
        if (leftpiv >= right) break;
      } else {
        memcpy(leftpiv,right,width);
        movect(1)
        leftpiv+=width;
        break;
      }
    }
    if (leftpiv<right) while (1) {
      if (fcmp(leftpiv,pivottemp)<=0) {
        leftpiv += width;
        if (leftpiv >= right) break;
      } else {
        memcpy(right,leftpiv,width);
        movect(1)
        right-=width;
        break;
      }
    }
  }
  memcpy(leftpiv,pivottemp,width);
  movect(1)
  prn=(leftpiv-(char *)base)/width;
  if (prn    > 1) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/*|| nelem-prn<=2*/)) // if the stack has grown too large, pass off to heap sort
      _heapsort(base,pivottemp,prn ,width,fcmp);
    else
      _introsort(depth+1,base,pivottemp,prn ,width,fcmp); // all evidence shows that there is no minimum value where heapsort performs better
  }
  if (nelem > prn+1  ) {
    if (INTROSORTENABLE && (depth>INTROSTACKLEVEL/* || nelem-prn<=2*/))
      _heapsort ((void *)((char *)base+(prn+1)*width),pivottemp,nelem-prn,width,fcmp);
    else
      _introsort(depth+1,(void *)((char *)base+(prn+1)*width),pivottemp,nelem-prn,width,fcmp);
  }
}

// introsort starts with quicksort and bails to heapsort when the stack gets too large
EXTERNC void qsortintro(void *base,size_t nelem,size_t width,int (QSORT_FCMP *fcmp)(const void *,const void *)) {
  if (nelem>1 && width>=1) {
    if (1 && width==sizeof(int)) {
      unsigned temp;
      _introsortint(0,(unsigned *)base,&temp,nelem,fcmp);
    } else {
      void *temp;
      if (temp=mallocsafe(width,"qsortqsort")) {
        _introsort(0,base,temp,nelem,width,fcmp);
        freesafe(temp,"qsortqsort");
      } else qsort(base,nelem,width,fcmp);
    }
  }
}
//#undef movect
//#undef setmoves
//#undef getmoves

#ifdef __MSDOS__
#define HANDLE int
#define INVALID_HANDLE_VALUE (-1)
#define FILE_ATTRIBUTE_NORMAL 0
// #include <stdio.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <dos.h>
// #include <io.h>
// #include <mem.h>
// #include <string.h>
// #include <stdlib.h>
// Virtually all of the fopen(), open(),_open,  _dos_open() and related calls are defective in some way
// we'll rewrite ones that work using the lib tools that are closest to fully functional
unsigned _readX(HANDLE handle,void far *buf,unsigned len) {
  unsigned nr;

  nr=0;
  errno=_dos_read(handle,buf,len,&nr);
  return(nr);
}

// lseek() then write() with 0 bytes to truncate
unsigned _truncateX(HANDLE handle,unsigned long len) {
  unsigned nr;
  unsigned long pos;

  nr=0;
  pos=tell(handle);
  lseek(handle,len,SEEK_SET);
  errno=_dos_write(handle,"",0,&nr);
  if (pos<len) lseek(handle,pos,SEEK_SET);
  return(nr);
}

// we assume _write() is defective too.
// prevent len=0 from truncating
unsigned _writeX(HANDLE handle,const void far *buf,unsigned len) {
  unsigned nr;

  nr=0;
  if (len) errno=_dos_write(handle,buf,len,&nr);
  return(nr);
}

HANDLE _creatX(const char *path,int attrib) {
  HANDLE hnd;

  hnd= INVALID_HANDLE_VALUE;
  errno=_dos_creat(path,attrib,&hnd);
  return(hnd);
}

HANDLE _openX(const char *filename,int oflags,int shflags) {
  HANDLE hnd;

  hnd= -1;
  errno=_dos_open(filename,oflags|shflags,&hnd);
  return(hnd);
}

int _closeX(HANDLE hnd) {
  errno=_dos_close(hnd);
  return errno?-1:0;
}
#else
// Windows File I/O routines are a hassle. Here's some wrappers that make them no harder to use than MS-DOS
#define O_DENYWRITE FILE_SHARE_WRITE
#define O_DENYREAD FILE_SHARE_READ
#define O_DENYALL (FILE_SHARE_READ|FILE_SHARE_WRITE)
#define O_DENYNONE 0
#define O_RDONLY GENERIC_READ
#define O_WRONLY GENERIC_WRITE
#define O_RDWR (GENERIC_READ|GENERIC_WRITE)
#define O_BINARY 0
#define _openX(filename,oflags,shflags) CreateFile(filename,oflags,shflags,NULL,OPEN_EXISTING,0,NULL)

#define _creatX(filename,attrib) CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,attrib,NULL)

EXTERNC DWORD _writeX(HANDLE handle,const void *buf,DWORD len) {
  DWORD nr; /* the help says this is init to 0 by WriteFile */

  WriteFile(handle,buf,len,&nr,NULL);
  return(nr);
}

EXTERNC DWORD _readX(HANDLE handle,void *buf,DWORD len) {
  DWORD nr;

  nr=0;
  ReadFile(handle,buf,len,&nr,NULL);
  return(nr);
}

#define _closeX(hnd) CloseHandle(hnd)
#endif

const char eoltypes[3][4]={"\r\n","\r","\n"}; // could be 3 but 4 makes the compiler use SHL,2 instead of IMUL*3

// Search through a string for quote symbols
// If starting at a non quote, will return the postion of the next quote, possibly end
// If starting at a quote, will return the position after the close quote, possibly end
// If starting at end, returns NULL
// style=0, detect c-string escaping \" or \' or \\ or \anycharacter
// style=1, detect vb-string escaping "" or sql escaping ''
// style=2, Foxpro-string: no escaping
// some routines use 0 for no string detection and style+1 for the styles
EXTERNC char *findnextquote(char *str,char *end,unsigned style) {
  unsigned dx;
  char cq[3];

  if (str>=end) return(NULL);
  if (*str=='"' || *str=='\'') {
    strcpy(cq,style?"'":"'\\");
    cq[0]=str[0];
    str++;
    dx=1;
  } else {
    strcpy(cq,"'\"");
    dx=0;
  }
  while(1) {
    str=memcspn(str,end,cq,strlen(cq));
    if (str>=end) return(end);
    if (!style) { // C
      if (*str!='\\') return(str+dx);
      str+=2;
    } else { // VB & Foxpro
      if (style==1 && str[0]==str[1]) str+=2;
      else return(str+dx);
    }
  }
}

// SUBSECTION End
// SECTION: END

// SECTION: Functions to test

#include "MicroXML.h"

//http://www.w3.org/TR/2004/REC-xml-20040204/

// 8 bit characters such as ANSI and UTF-8 are supported
// 16 bit characters aren't
// It's not possible for the XML parser to do any conversion since it cannot know
// what the source and destinations formats are.

// The other XML parsers are coded in C++ with no thought to performance or sensibility so I had to rewrite
// This implements high speed serial XML parsing only which is the only kind we need and what all XML parsing
// is based on.

// This won't read OOo documents because OOo writes text after close tags and I cannot find
// a standard that allows this.

void XMLXlatFromEntity(struct MICROXML *pmx) {
  unsigned uValueConvertedLen=0,uValueConvertedSz=pmx->uValueConvertedSz;
  char *sValueConverted=pmx->sValueConverted;
  char *sValue=pmx->sValue,*sValueEnd=sValue+pmx->uValueLen;
  char *sValueP=sValue;
  while((sValue=memchrX(sValue,sValueEnd,'&'))<sValueEnd) {
    char *sSemiColon;
    if (sValue>sValueP) {memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,sValueP,sValue-sValueP,"XMLXlatFromEntity"); if (!sValueConverted) goto fail;}
    sValue++;
    sSemiColon=memchrX(sValue,sValueEnd,';');
    if (sSemiColon<sValueEnd) {
      unsigned char cRes[1];
      if (*sValue=='#') {
        unsigned long ulVal;
        int radix;
        char *sEnder;
        sValue++;
        if (*sValue=='x') {
          sValue++;
          radix=16;
        } else radix=10;
        ulVal=strtoul(sValue,&sEnder,radix);
        if (ulVal>=256) goto fail2;
        *cRes=ulVal;
      } else {
        unsigned uEntityLen=sSemiColon-sValue;
        switch(uEntityLen) {
        case 2:
               if (!memicmp("lt",sValue,2)) *cRes='<';
          else if (!memicmp("gt",sValue,2)) *cRes='>';
          else goto fail2;
          break;
        case 3:
          if (!memicmp("amp",sValue,3)) *cRes='&';
          else goto fail2;
          break;
        case 4:
               if (!memicmp("quot",sValue,4)) *cRes='"';
          else if (!memicmp("apos",sValue,4)) *cRes='\'';
          else goto fail2;
          break;
        }
      }
      memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,cRes,sizeof(cRes),"XMLXlatFromEntity"); if (!sValueConverted) goto fail;
      sValue=sSemiColon+1;
    }
fail2:
    sValueP=sValue;
  }
  if (sValue>sValueP) {memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,sValueP,sValue-sValueP,"XMLXlatFromEntity"); if (!sValueConverted) goto fail;}
  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"",1,"XMLXlatFromEntity");
  uValueConvertedLen--;
fail:
  pmx->sValueConverted=sValueConverted;
  pmx->uValueConvertedSz=uValueConvertedSz;
  pmx->uValueConvertedLen=uValueConvertedLen;
}

// returns malloc'd string to be free'd
// returns NULL if there was nothing to convert and the original string should be used
char *strdupXMLXlatToEntity(char *sValue,unsigned uValueLen,unsigned *puDestLen) {
  char *sValueConverted=NULL;
  unsigned uValueConvertedLen=0;
  if (uValueLen) {
    unsigned uValueConvertedSz=0;
    char *sValueEnd=sValue+uValueLen;
    char *sValueP;
    unsigned quick[256/(sizeof(unsigned)*8)];
    if (sValue<sValueEnd && *sValue<32) { // convert the first control character to a numeric entity
      char *szNumCode=smprintf("&#%u;",*sValue);
      if (szNumCode) {
        memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,szNumCode,-1,"strdupXMLXlatToEntity");
        freesafe(szNumCode,"strdupXMLXlatToEntity");
        if (!sValueConverted) goto fail;
        sValue++;
      }
    }
    sValueP=sValue;
    memcqspnstart("&<>\"'",6,quick); // including \0
    while((sValue=memcqspn(sValue,sValueEnd,quick))<sValueEnd) {
      if (sValue>sValueP) {memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,sValueP,sValue-sValueP,"strdupXMLXlatToEntity"); if (!sValueConverted) goto fail;}
      switch(*sValue) {
      case 0:    memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&#0;"  ,4,"strdupXMLXlatToEntity"); break;
      case '&':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&amp;" ,5,"strdupXMLXlatToEntity"); break;
      case '"':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&quot;",6,"strdupXMLXlatToEntity"); break;
      case '<':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&lt;"  ,4,"strdupXMLXlatToEntity"); break;
      case '>':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&gt;"  ,4,"strdupXMLXlatToEntity"); break;
      case '\'': memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&apos;",6,"strdupXMLXlatToEntity"); break;
      }
      if (!sValueConverted) goto fail;
      sValueP= ++sValue;
    }
    if (sValueConverted && sValue>sValueP) {memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,sValueP,sValue-sValueP,"strdupXMLXlatToEntity"); if (!sValueConverted) goto fail;}
fail:
  }
  *puDestLen=uValueConvertedLen;
  return sValueConverted;
}

// For efficiency, we want the smallest possible buffer.
// Unfortunately, the buffer will need to grow to the largest text block.

// My buffering code is about 1/500'th the size of istream (500KB/1KB)
// Which means my code should run 500-5000 times faster.

// uMinSize>0 means we must guarantee at least uMinSize more characters
// are available for scanning. This is calculated from psFrom or _psBufBegin if psFrom==NULL.

// uMinSize==0 means we are at the end of the buffer and need more
// which will double the buffer size if at least half can't be expired

// returns the amount that needs to be added to the caller's pointers
// including changes from realloc(_sBuf) and adjustment of _psBufBegin
// due to expiration of data
int XMLBufMin(struct MICROXML *pmx,unsigned uMinSize,char *psFrom) {
  unsigned uBufLen;
  int rviExpire,rviRealloc;
  rviExpire=pmx->_psBufBegin-pmx->_sBuf;
  if ((unsigned)rviExpire>=pmx->_uSbufSz/2) {
    memmove(pmx->_sBuf,pmx->_psBufBegin,pmx->_psBufEnd-pmx->_psBufBegin);
    rviExpire = -rviExpire;
    pmx->_psBufBegin += rviExpire;
    pmx->_psBufEnd += rviExpire;
  } else {
    if (!uMinSize) uMinSize=pmx->_uSbufSz+1; // doubled by ARMSTRATEGY_INCREASE
    uMinSize += rviExpire;
    rviExpire=0;
  }
  if (psFrom) {
    psFrom += rviExpire;
    uMinSize += psFrom-pmx->_psBufBegin;
  }
  rviRealloc=armreallocsafe(&pmx->_sBuf,&pmx->_uSbufSz,uMinSize,ARMSTRATEGY_INCREASE,FALSE,"XMLBufMin");
  if (rviRealloc) {
    pmx->_psBufBegin += rviRealloc;
    pmx->_psBufEnd += rviRealloc;
  }
  if (pmx->_sBuf) {
    uBufLen=pmx->_psBufEnd-pmx->_sBuf;
    if (pmx->_uSbufSz>uBufLen) {
      unsigned uBw = _readX(pmx->_hFileHandle,pmx->_psBufEnd,pmx->_uSbufSz-uBufLen);
      pmx->_psBufEnd += uBw;
    }
  }
  return(rviExpire+rviRealloc);
}

int XMLRead(struct MICROXML *pmx) {
  if (pmx->_iState<=MX_DONE) return pmx->_iState;
  pmx->uLevel += pmx->iLevelDeferred;
  pmx->iLevelDeferred=0;
  do {
    pmx->_psBufBegin=memspn(pmx->_psBufBegin,pmx->_psBufEnd,"\r\n\t ",5); // skip past whitespace, including \0
    if (pmx->_psBufBegin<pmx->_psBufEnd) break;
    XMLBufMin(pmx,0,NULL); if (!pmx->_sBuf) goto fail;
    if (pmx->_psBufBegin >= pmx->_psBufEnd) {
      if (pmx->uLevel) goto fail;
      else {
        pmx->_iState=MX_DONE;
        return MX_DONE;
      }
    }
  } while(1); // on rare occasion these loops can leave the buffer exactly at the end
  switch(pmx->_iState) {
  case MX_TEXT: {
      int iNextState;
      unsigned uLeafLen;
      char *psRunner=pmx->_psBufBegin;
      if (*psRunner != '<') goto fail;
      pmx->_psBufBegin= ++psRunner;
      psRunner+=XMLBufMin(pmx,3,psRunner); if (!pmx->_sBuf) goto fail;
      if (*psRunner=='!' /* && !memcmp(psRunner+1,"--",2)*/) {
        char *psRecursive=pmx->_psBufBegin= (++psRunner); //+=3);
        do {
          int dx;
          psRunner=memchrX(psRunner,pmx->_psBufEnd,'>'); // memstr(..."-->",3);
          if (psRunner<pmx->_psBufEnd) {
            psRecursive=memstr(psRecursive,psRunner,"<!",2); // non buffered
            if (psRecursive==psRunner) break;
            psRunner++;
            psRecursive+=2;
          }
          dx=XMLBufMin(pmx,0,psRunner);
          psRunner+=dx;
          psRecursive+=dx;
          if (!pmx->_sBuf || psRunner >= pmx->_psBufEnd) goto fail;
        } while(1);
        pmx->uValueLen=psRunner-pmx->_psBufBegin;
        pmx->sValue=pmx->_psBufBegin;
        *psRunner='\0';
        psRunner++; //+=3; // at this point psRunner may not point to a valid buffer but this won't matter as long as the next call is guaranteed to adjust it
        iNextState=MX_COMMENT;
      } else {
        if (*psRunner=='/') {
          pmx->_psBufBegin= ++psRunner;
          iNextState=MX_LEVELDOWN;
        } else iNextState=MX_LEVELUP;
        do {
          psRunner=memcspn(psRunner,pmx->_psBufEnd,"\r\n\t '\"<>/",10); // including \0
          if (psRunner<pmx->_psBufEnd) break;
          psRunner+=XMLBufMin(pmx,0,NULL);
          if (!pmx->_sBuf || psRunner >= pmx->_psBufEnd) goto fail;
        } while(1);
        psRunner+=XMLBufMin(pmx,1,psRunner); if (!pmx->_sBuf) goto fail;
        uLeafLen=psRunner-pmx->_psBufBegin;
        if (iNextState==MX_LEVELDOWN) {
          char *psOldLeaf;
          unsigned uOldLeafLen;
          if (*psRunner != '>' || !pmx->uLevel || uLeafLen != pmx->uLeafLen || memcmp(pmx->_psBufBegin,pmx->sLeaf,uLeafLen) || !pmx->sPath) goto fail;
          if (!pmx->sLeaf[uLeafLen+1]) iNextState=MX_LEVELDOWNSAME;
          pmx->sValue=pmx->sLeaf;
          pmx->uValueLen=uLeafLen;
          pmx->uPathLen -= uLeafLen+1;
          pmx->sPath[pmx->uPathLen]='\0';
          for(uOldLeafLen=0,psOldLeaf=pmx->sPath+pmx->uPathLen-1; psOldLeaf>=pmx->sPath && psOldLeaf[0]!='/'; psOldLeaf--,uOldLeafLen++);
          pmx->sLeaf=psOldLeaf+1;
          pmx->uLeafLen=uOldLeafLen;
          pmx->iLevelDeferred= -1;
          pmx->_iState=MX_TEXT;
          psRunner++;
        } else {
          unsigned uLeafPos;
          if (memchr("'\"<",*psRunner,4)) goto fail; // including \0
          memcpyarmsafe(&pmx->sPath,&pmx->uPathSz,&pmx->uPathLen,"/",1,"XMLRead"); if (!pmx->sPath) goto fail;
          uLeafPos=pmx->uPathLen; // relative position in case the buffering changes the base
          memcpyarmsafe(&pmx->sPath,&pmx->uPathSz,&pmx->uPathLen,pmx->_psBufBegin,uLeafLen,"XMLRead"); if (!pmx->sPath) goto fail;
          memcpyarmsafe(&pmx->sPath,&pmx->uPathSz,&pmx->uPathLen,"\0",2,"XMLRead"); /* 2 supports MX_LEVELDOWNSAME */ if (!pmx->sPath) goto fail;
          pmx->uPathLen-=2;
          pmx->sValue=pmx->sLeaf=pmx->sPath+uLeafPos;
          pmx->uValueLen=pmx->uLeafLen=uLeafLen;
          pmx->iLevelDeferred=1;
          pmx->_iState=MX_ATTRIBUTE;
        }
      }
      pmx->sAttribute=NULL;
      pmx->uAttributeLen=0;
      pmx->_psBufBegin = psRunner;
      return iNextState;
    }
  case MX_ATTRIBUTESPECIAL:
    XMLBufMin(pmx,2,NULL); if (!pmx->_sBuf) goto fail;
    if (!memcmp(pmx->_psBufBegin,"?>",2)) pmx->_psBufBegin[0]='/';
  case MX_ATTRIBUTE: {
      XMLBufMin(pmx,2,NULL); if (!pmx->_sBuf) goto fail;
      if (memchr("'\"<?",pmx->_psBufBegin[0],5)) goto fail; // including \0
      if (!memcmp(pmx->_psBufBegin,"/>",2)) { // self close tag
        pmx->_psBufBegin += 2;

        if (MX_ATTRIBUTESPECIAL != pmx->_iState) {
          unsigned uLeafLen;
          char *psOldLeaf;
          unsigned uOldLeafLen;
          uLeafLen=pmx->uLeafLen;
          pmx->uPathLen -= uLeafLen+1;
          pmx->sPath[pmx->uPathLen]='\0';
          for(uOldLeafLen=0,psOldLeaf=pmx->sPath+pmx->uPathLen-1; psOldLeaf>=pmx->sPath && psOldLeaf[0]!='/'; psOldLeaf--,uOldLeafLen++);
          pmx->sLeaf=psOldLeaf+1;
          pmx->uLeafLen=uOldLeafLen;
          pmx->uLevel--;
        }

        pmx->_iState=MX_TEXT;
        pmx->sValue=NULL;
        pmx->uValueLen=0;
        pmx->sAttribute=NULL;
        pmx->uAttributeLen=0;
        return MX_LEVELDOWNSAME;
      } else if (pmx->_psBufBegin[0]=='>') {
        char *psRunner;
        pmx->_psBufBegin++;
        do {
          pmx->_psBufBegin=memspn(pmx->_psBufBegin,pmx->_psBufEnd,"\r\n\t ",5); // skip past whitespace, including \0
          if (pmx->_psBufBegin<pmx->_psBufEnd) break;
          XMLBufMin(pmx,0,NULL);
          if (!pmx->_sBuf || pmx->_psBufBegin >= pmx->_psBufEnd) goto fail;
        } while(1);
        do {
          psRunner=memchrX(pmx->_psBufBegin,pmx->_psBufEnd,'<');
          if (psRunner<pmx->_psBufEnd) break;
          psRunner+=XMLBufMin(pmx,0,NULL);
          if (!pmx->_sBuf || psRunner >= pmx->_psBufEnd) goto fail;
        } while(1);
        pmx->sValue=(pmx->uValueLen=psRunner-pmx->_psBufBegin)?pmx->_psBufBegin:"";
        XMLXlatFromEntity(pmx);
        // find the text
        pmx->_psBufBegin = psRunner;
        pmx->_iState=MX_TEXT;
        pmx->sAttribute=NULL;
        pmx->uAttributeLen=0;
        return MX_TEXT;
      } else {
        unsigned uValue;
        char *psRunner;
        do {
          psRunner=memcspn(pmx->_psBufBegin,pmx->_psBufEnd,"\r\n\t '\"<>=",10); // find the =
          if (psRunner<pmx->_psBufEnd) break;
          psRunner+=XMLBufMin(pmx,0,NULL);
          if (!pmx->_sBuf || psRunner >= pmx->_psBufEnd) goto fail;
        } while(1);
        pmx->uAttributeLen = psRunner-pmx->_psBufBegin;
        psRunner+=XMLBufMin(pmx,2,psRunner); if (!pmx->_sBuf) goto fail;
        if (*psRunner != '=') goto fail;
#if NPPDEBUG
        *psRunner='\0';
#endif
        psRunner++;
        if (*psRunner != '"' && *psRunner != '\'') goto fail;
        pmx->cQuote=(unsigned char)psRunner[0];
        /*unsigned */uValue=psRunner - pmx->_psBufBegin+1; // we can't expire the attribute here
        {
        char *psQuote;
        do { // findnextquote isn't restartable in the middle
          int dx;
          psQuote=findnextquote(psRunner,pmx->_psBufEnd,2);
          if (psQuote<pmx->_psBufEnd) break;
          dx=XMLBufMin(pmx,0,NULL);
          psRunner+=dx;
          psQuote+=dx;
          if (!pmx->_sBuf || psRunner >= pmx->_psBufEnd) goto fail;
        } while(1);
        psRunner=psQuote-1;
        }
        pmx->uValueLen=psRunner - pmx->_psBufBegin - uValue;
#if NPPDEBUG
        *psRunner='\0';
#endif
        pmx->sAttribute=pmx->_psBufBegin; // buffering may change _psBufBegin during operation so we use relative values
        pmx->sValue=pmx->_psBufBegin+uValue;
        XMLXlatFromEntity(pmx);
        //MessageBoxFree(0,smprintf(TEXT("%s[%u]=\"%s\"[%u]"),pmx->sAttribute,pmx->uAttributeLen,pmx->sValue,pmx->uValueLen),TEXT("???"),MB_OK);
        pmx->_psBufBegin=++psRunner;
        return MX_ATTRIBUTE;
      }
    }
  }
fail:
  pmx->_iState=MX_FAIL;
  return MX_FAIL;
}

void XMLReadClose(struct MICROXML *pmx) {
  if (pmx->_hFileHandle) {
    _closeX(pmx->_hFileHandle);
    if (pmx->sValueConverted) freesafe(pmx->sValueConverted,"XMLReadClose");
    if (pmx->_sBuf) freesafe(pmx->_sBuf,"XMLReadClose");
    if (pmx->sPath) freesafe(pmx->sPath,"XMLReadClose");
    memset(pmx,0,sizeof(*pmx));
  }
}

// returns TRUE if XML file was opened
// read-write not supported
BOOL XMLReadOpen(struct MICROXML *pmx,char *fn,int oflags,int shflags) {
  memset(pmx,0,sizeof(*pmx));
  if (INVALID_HANDLE_VALUE != (pmx->_hFileHandle=_openX(fn,oflags,shflags))) {
    if (!(pmx->_sBuf=(char *)mallocsafe(pmx->_uSbufSz=1,"XMLReadOpen"))) goto fail;
    pmx->_psBufEnd=pmx->_psBufBegin=pmx->_sBuf;
    XMLBufMin(pmx,6,NULL);
    if (!pmx->_sBuf || pmx->_psBufEnd-pmx->_psBufBegin<=6) goto fail;
    if (!memcmp(pmx->_psBufBegin,"<?xml ",6)) {
      int iResult;
      pmx->_psBufBegin += 6;
      pmx->_iState=MX_ATTRIBUTESPECIAL;
      while(MX_ATTRIBUTE==(iResult=XMLRead(pmx))) switch(pmx->uAttributeLen) {
      case 7: if (!memcmp(pmx->sAttribute,"version",7)) strncpymem(pmx->sVersion,NELEM(pmx->sVersion),pmx->sValue,pmx->uValueLen); break;
      case 8: if (!memcmp(pmx->sAttribute,"encoding",8)) strncpymem(pmx->sEncoding,NELEM(pmx->sEncoding),pmx->sValue,pmx->uValueLen); break;
      }
      //MessageBoxFree(0,smprintf(TEXT("Version:%s\r\nEncoding:%s"),pmx->sVersion,pmx->sEncoding),TEXT("???"),MB_OK);
      if (iResult==MX_FAIL) goto fail;
    }
    pmx->_iState=MX_TEXT;
    return TRUE;
fail:
    XMLReadClose(pmx);
  }
  return FALSE;
}

static char g_sTabs[16]="\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
void XMLWriteClose(HANDLE fo) {
  _writeX(fo,"\n",1);
  _closeX(fo);
}
void XMLWriteLevelDown(HANDLE fo,unsigned uLevel) {
  _writeX(fo,"\n",1);
  while(uLevel) {
    unsigned uToWrite=(uLevel>sizeof(g_sTabs))?sizeof(g_sTabs):uLevel;
    _writeX(fo,g_sTabs,uToWrite); uLevel-=uToWrite;
  }
}

void XMLWriteLevelUp(HANDLE fo,unsigned uLevel,char *sTag,unsigned uTagLen) {
  if (uTagLen == (unsigned)-1) uTagLen=strlen(sTag);
  XMLWriteLevelDown(fo,uLevel);
  _writeX(fo,"<",1);
  _writeX(fo,sTag,uTagLen);
}

void XMLWriteLevelDownSame(HANDLE fo,char *sTag,unsigned uTagLen) {
  if (sTag) {
    if (uTagLen == (unsigned)-1) uTagLen=strlen(sTag);
    _writeX(fo,"</",2);
    _writeX(fo,sTag,uTagLen);
    _writeX(fo,">",1);
  } else _writeX(fo,"/>",2);
}

void XMLWriteAttribute(HANDLE fo,char *sAttribute,unsigned uAttributeLen,char *sValue,unsigned uValueLen,unsigned cQuote) {
  char *sValueConverted;
  unsigned uValueConvertedLen;
  unsigned char sQuote[1];
  if (uAttributeLen == (unsigned)-1) uAttributeLen=strlen(sAttribute);
  if (uValueLen == (unsigned)-1) uValueLen=strlen(sValue);
  sQuote[0]=cQuote;
  _writeX(fo," ",1);
  _writeX(fo,sAttribute,uAttributeLen);
  _writeX(fo,"=",1);
  _writeX(fo,sQuote,1);
  sValueConverted = strdupXMLXlatToEntity(sValue,uValueLen,&uValueConvertedLen);
  if (sValueConverted) {
    _writeX(fo,sValueConverted,uValueConvertedLen);
    freesafe(sValueConverted,"XMLWriteAttribute");
  } else _writeX(fo,sValue,uValueLen);
  _writeX(fo,sQuote,1);
}

// The XML standard sucks in many ways, one of which is detecting blank text by whitespace
// a Prefix and suffix string is necessary because leading whitespace must be removed from TEXT to figure out if it is blank
// '/' is an excellent character to use
// szPfxSfx may be NULL if there is no need to preserve whitespace
void XMLWriteText(HANDLE fo,char *sValue,unsigned uValueLen,char *szPfxSfx) {
  char *sValueConverted;
  unsigned uValueConvertedLen;
  unsigned uPfxSfxLen;
  _writeX(fo,">",1);
  if (uValueLen == (unsigned)-1) uValueLen=strlen(sValue);
  sValueConverted = strdupXMLXlatToEntity(sValue,uValueLen,&uValueConvertedLen);
  if (szPfxSfx) _writeX(fo,szPfxSfx,uPfxSfxLen=strlen(szPfxSfx));
  if (sValueConverted) {
    _writeX(fo,sValueConverted,uValueConvertedLen);
    freesafe(sValueConverted,"XMLWriteText");
  } else _writeX(fo,sValue,uValueLen);
  if (szPfxSfx) _writeX(fo,szPfxSfx,uPfxSfxLen);
}

// What if comments contain -->
void XMLWriteComment(HANDLE fo,unsigned uLevel,char *sValue,unsigned uValueLen) {
  XMLWriteLevelDown(fo,uLevel);
  _writeX(fo,"<!",2);
  _writeX(fo,sValue,uValueLen);
  _writeX(fo,">",1);
}

HANDLE XMLWriteCreat(char *szfn,char *szVersion,char *szEncoding) {
  HANDLE fo;
  if (INVALID_HANDLE_VALUE != (fo=_creatX(szfn,0))) {
    _writeX(fo,"<?xml version=\"",15);
    _writeX(fo,szVersion,strlen(szVersion));
    _writeX(fo,"\" encoding=\"",12);
    _writeX(fo,szEncoding,strlen(szEncoding));
    _writeX(fo,"\"?>",3);
  }
  return fo;
}

#if NPPDEBUG
// parses and duplicates XML from fn1 to fn2, use a diff tool to check accuracy
void XMLTest(char *fn1,char *fn2) {
  struct MICROXML mx;
  int i;
  int res;
  //FILE *fo=fopen("C:\\test2.xml","w");
  if (XMLReadOpen(&mx,fn1,O_RDONLY,O_DENYWRITE)) {
    HANDLE fo2=XMLWriteCreat(fn2,mx.sVersion,mx.sEncoding);
#ifdef __MSDOS__
    printf("<?xml version=\"%s\" encoding=\"%s\" ?>",mx.sVersion,mx.sEncoding);
#endif
    for(i=0; i<10000; i++) {
    switch(XMLRead(&mx)) {
    case MX_FAIL:
#ifdef __MSDOS_
      printf("Failed!\n");
#else
      MessageBox(0,"Failure","XML",MB_OK);
#endif
    case MX_DONE:
      goto fail;
    case MX_LEVELUP:
      XMLWriteLevelUp(fo2,mx.uLevel,mx.sValue,mx.uValueLen);
#ifdef __MSDOS__
      printf("\n%*s<%s",mx.uLevel,"",mx.sValue);
#endif
      break;
    case MX_LEVELDOWN:
      XMLWriteLevelDown(fo2,mx.uLevel);
#ifdef __MSDOS__
      printf("\n%*s",mx.uLevel,"");
#endif
    case MX_LEVELDOWNSAME:
      XMLWriteLevelDownSame(fo2,mx.sValue,mx.uValueLen);
#ifdef __MSDOS__
      if (mx.sValue) printf("</%s>",mx.sValue);
      else printf("/>");
#endif
      break;
    case MX_ATTRIBUTE:
      XMLWriteAttribute(fo2,mx.sAttribute,mx.uAttributeLen,mx.sValueConverted,mx.uValueConvertedLen,mx.cQuote);
#ifdef __MSDOS__
      printf(" %s=\"%s\"",mx.sAttribute,mx.sValue);
#endif
      break;
    case MX_TEXT:
      XMLWriteText(fo2,mx.sValueConverted,mx.uValueConvertedLen,NULL);
#ifdef __MSDOS__
      printf(">%s",mx.sValueConverted);
#endif
      break;
    case MX_COMMENT:
      XMLWriteComment(fo2,mx.uLevel,mx.sValue,mx.uValueLen);
#ifdef __MSDOS__
      printf("\n%*s<!%s>",mx.uLevel,"",mx.sValue);
#endif
      break;
    }
    }
fail:
#ifdef __MSDOS__
    puts("");
#endif
    XMLReadClose(&mx);
    XMLWriteClose(fo2);
  }
  //fclose(fo);
}
#endif

void loadXML(char *lpstrFile) {
        struct MICROXML mx;
          if (XMLReadOpen(&mx,/*(g_ofn.*/lpstrFile,O_RDONLY,O_DENYWRITE)) {
          int iXMLRead;
          while((iXMLRead=XMLRead(&mx))>MX_DONE && (mx.uLevel>0 || iXMLRead>=MX_DOWNLIMIT)) if (mx.uLevel==0) switch(iXMLRead) {
          case MX_LEVELUP:
            if (mx.uValueLen == 14 && !memicmp(mx.sValue,"TextFX:Replace",14)) while((iXMLRead=XMLRead(&mx))>MX_DONE && (mx.uLevel>1 || iXMLRead>=MX_DOWNLIMIT)) if (mx.uLevel==1) switch(iXMLRead) {
            case MX_ATTRIBUTE:
              switch(mx.uAttributeLen) {
              case 16:
                if (!memicmp(mx.sAttribute,"option:MatchCase",16)) {MessageBox(0,"option:MatchCase","???",MB_OK); goto ok;}
                else if (!memicmp(mx.sAttribute,"option:WholeWord",16)) {MessageBox(0,"option:WholeWord","???",MB_OK); goto ok;}
                break;
              }
              MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("#1 %u Discard: %u"),mx.uLevel,iXMLRead),TEXT("Find & Replace"),MB_OK);
ok:           break;
            case MX_LEVELUP:
              switch(mx.uValueLen) {
              case 8: if (!memicmp(mx.sValue,"FindText",8)) MessageBox(0,"FindText","???",MB_OK); break;
              case 11: if (!memicmp(mx.sValue,"ReplaceText",11)) MessageBox(0,"ReplaceText","???",MB_OK); break;
              }
              break;
            default:
              MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("#1 %u Discard: %u"),mx.uLevel,iXMLRead),TEXT("Find & Replace"),MB_OK);
              break;
            }
            break;
          default:
              MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("#0 %u Discard: %u"),mx.uLevel,iXMLRead),TEXT("Find & Replace"),MB_OK);
              break;
          }
          if (iXMLRead!=MX_DONE) MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("Failure: [%u]%s"),mx.sValue,mx.uValueLen),TEXT("Find & Replace"),MB_OK);
          XMLReadClose(&mx);
        }
}



// SECTION: END

#pragma warn -par /* Borland-C warning: parameter is never used */

#if 0
void AddTest(void) {
  AddDlgModeless(5);
  AddDlgModeless(6);
  RemoveDlgModeless(5);
  RemoveDlgModeless(6);

#if 0
  FINDREPLACE **pfrl=NULL;
  HWND *phwndl=NULL;
  unsigned cfrl=0;

  AddHwnd(&phwndl,&pfrl,&cfrl,50,(void *)0x51);
  AddHwnd(&phwndl,&pfrl,&cfrl,40,(void *)0x41);
  printf("40:%p\n",FindHwnd(phwndl,pfrl,cfrl,40,NULL));
  printf("50:%p\n",FindHwnd(phwndl,pfrl,cfrl,50,NULL));
  RemoveHwnd(&phwndl,&pfrl,&cfrl,50);
  RemoveHwnd(&phwndl,&pfrl,&cfrl,40);
  RemoveHwnd(&phwndl,&pfrl,&cfrl,60);
#endif
}
#endif

unsigned long fcmpct;
unsigned minstack;
#define DATATYPE unsigned long
int __cdecl fcmpx(const void *av,const void *bv) {
  DATATYPE a=*(unsigned*)av,b=*(unsigned*)bv;
  fcmpct++;
  if (minstack>_SP) minstack=_SP;
  return(a<b?-1:((a==b)?0:1));
}

#define NUM_ITEMS 3000
DATATYPE test[NUM_ITEMS];
void mergedemo(void) {
  unsigned i;

DATATYPE orig[NUM_ITEMS],qs[NUM_ITEMS];

  //seed random number generator
  randomize(); //srand(getpid());

  //fill array with random integers
  for (i = 0; i < NUM_ITEMS; i++) orig[i] = rand();

  //perform merge sort on array
  minstack=-1; setmoves(0) fcmpct=0; memcpy(qs  ,orig,sizeof(orig)); qsort(qs,NUM_ITEMS,sizeof(qs[0]),fcmpx); memcpy(test,qs,sizeof(test));
  printf("%s-%s: minstack:%X moves:%lu fcmps:%5lu\n","qs","????",minstack,getmoves(),fcmpct);
  minstack=-1; setmoves(0) fcmpct=0; memcpy(test,orig,sizeof(orig)); qsortintro(test,NUM_ITEMS,sizeof(test[0]),fcmpx);
  printf("%s-%s: minstack:%X moves:%lu fcmps:%5lu\n","my",memcmp(qs,test,sizeof(qs))?"FAIL":"PASS",minstack,getmoves(),fcmpct);
  if (NUM_ITEMS<10) for (i = 0; i < NUM_ITEMS; i++) printf("%8i %8i\n",qs[i],test[i]);
  minstack=-1; setmoves(0) fcmpct=0; memcpy(test,orig,sizeof(orig)); qsortmerge(test,NUM_ITEMS,sizeof(test[0]),fcmpx);
  printf("%s-%s: minstack:%X moves:%lu fcmps:%5lu\n","mg",memcmp(qs,test,sizeof(qs))?"FAIL":"PASS",minstack,getmoves(),fcmpct);
  minstack=-1; setmoves(0) fcmpct=0; memcpy(test,orig,sizeof(orig)); qsortheap(test,NUM_ITEMS,sizeof(test[0]),fcmpx);
  printf("%s-%s: minstack:%X moves:%lu fcmps:%5lu\n","hp",memcmp(qs,test,sizeof(qs))?"FAIL":"PASS",minstack,getmoves(),fcmpct);
  if (NUM_ITEMS<10) for (i = 0; i < NUM_ITEMS; i++) printf("%8i %8i\n",qs[i],test[i]);

  printf("\nDone with sort Items=%u size=%u\n",NUM_ITEMS,sizeof(qs));

}

int main(int argc,char *argv[]) {
  char sts[]=
/*"AAA</tr></tr><b>&#36;2005-08-29</b><br>Test\nTest2<br>\n T\n"
"<img src=\"gfx/o.gif\" width=9 height=9 alt=\"o\"> A new utilities for <b>KMK/JZ.</b> hdd interface: <a href=\"http://drac030.krap.pl/fdisk2.arc\">FDISK II 2.1</a> (<b>update!</b>),<a href=\"http://drac030.krap.pl/kmkdiag.arc\">KMK/JZ Diag 1.1</a> and <a href=\"http://drac030.krap.pl/hdbios19.arc\">IDE Bios 1.9</a>. More info on <a href=\"http://drac030.atari8.info\">Draco's site</a> in download section.</p>\n"
" t <html> \n\n\n<tag> Depech&pizza;&nbsp;Mode<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"  <html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"<head>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"></meta>\n"
"<style type=\"text/css\" media=\"screen\">\n"
"  body,table,input {\n"
"    font-family: verdana, arial, helvetica, geneva, swiss, sunsans-regular, sans-serif;\n"
"    font-size: 10pt;\n"
"  }\n"
"</style>\n"
"<title>TopDog Pet&nbsp;Care Online&copy;&Gt;&lt;&quot;</title>\n"
"</head>\n"
"<!--#include file=\"style.htm\"-->hello Captain & Tenneile"*/
/*    "NPPTextFX"
    "NPPTextFX,  a GPL \n\n \n\n Plugin for Notepad++ by Chris Severance and other authors.\t"
    "Perform a variety of conversions on selected text. \r\n\r\n\r\n"
    "If\tthere is\nnot enough\tmemory to complete,\ttext still needing changes will be left marked.\n"*/
//    "Some menu choices will show help if no text is selected or the Selection is empty\r\n\r\n \r\n\r\n"
//    "printf(\"String1\");<var>;\n\"quo'ted\\\"str'ing2\"\0;variable;'\r\nstring\"3\"'\"juxtastring\")" // c-string
/*  "*str=='\\'') {\n"
  "  strcpy(cq,style?\"'\\\\\":\"'\");\n"
  "  strcpy(cq,\"'\\\"\");\n"
  "    if (*str!='\\\\') return(str+dx);\n"*/
/*    "FILLTEXT\n"
    "case 'c'; num=strtol(str,NULL,0); break;\n"
    "case 'd'; num=strtol(str,NULL,10); break;\n"
    "LN TOO SHRT;\n\n"
    "case 'h'; num=strtol(str,NULL,16); break;\n"
    "case 'b'; num=strtol(str,NULL,2); break;\n"
    "case 'o'; num=strtol(str,NULL,8); break; \n\n\n\n\n\n"*/
//    "\"Here\\l\"\"ies\\'ethyl\\\",\"\\gone\\\"\n"
//    "2.6 4.7 65.61 8\n-10000 +.09"
/*    "case 9: strcpy(str,\"T,B\"); strcpy(lit,\"\\\\t\"); break;\n"
    "case 10: strcpy(str,\"LF\"); strcpy(lit,\"\\\\n\"); break;\n"
    "case '\\\"': strcpy(lit,\"\\\\\\\"\"); break;\n"*/
//  "\n\nAA\r\n  A\n\n    a\n  AAAA\n"
//    "\t    \n \t hello\n"
//     "test  \ntest 2\ntest 3    "
  //"#define XXX\\\nxyzzy\\\npdq\nvoid (test) {\r\n/* test\n\ttest 2*/\nif ((a) || // \"\r\n(b) ) {\n\t\tprintf('t\'{est');\r\n\t}\r\n}\n"
/*"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0\n"
"aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1\n"
"c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0\n"
"aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdl\n"
"LCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=\n"*/
//"ZDpcQlxCSU5cVEQuRVhFIEhUTUwuRVhF"
//"  3F2c,458D,\"aa,,aa\\\"\"8651,4c2f\n"
  "2,3,4, 5"
;
  char *tx=NULL;
  unsigned txsz=0,destlen,maxexpand;
  char /**fail=NULL,*/*rvc=NULL;
  //unsigned nm[512]
  unsigned rvu=0;
  unsigned i=0;

  clrscr();
#ifdef mallocsafeinit
  mallocsafeinit();
#endif
  printf("\nStart:\n");

  //static char test[16],test2[]="Hello There How Are You";
  //strncpymem(test,sizeof(test),test2,strlen(test2));
  //char test[]="Hello|\n\t";
  //puts(memcspn(test,test+strlen(test),"\r\n\t|",4));
  loadXML("c:\\PROGRA~1\\NOTEPA~1\\plugins\\NPPTEX~1\\REPLAC~1\\ZORK.XML");

#if 0
  HistoryInit(&g_FindHist,0);
  HistoryInit(&g_ReplHist,0);
  HistoryAddString(&g_FindHist,0,"AAAAA",6,"???");
  HistoryAddString(&g_FindHist,0,"BBBBB",6,"???");
  HistoryAddString(&g_FindHist,0,"AAaaa",6,"???");
  HistoryAddString(&g_FindHist,0,"CCCCC",6,"???");
  HistoryAddString(&g_FindHist,0,"CCCCCDDDDD",11,"???");
  HistoryAddString(&g_FindHist,0,"AAAAA",6,"???");
  HistoryInit(&g_FindHist,1);
  HistoryInit(&g_ReplHist,1);
#endif

  //XMLTest("C:\\test.xml","C:\\test3.xml");

  //testmallocsafe();
  //powcheck(57); powcheck(32); powcheck(0);

  //NumberGuess();
  //AddTest();
  //mergedemo(); mallocsafedone(); exit(1);
    txsz=0?10000:strlen(sts)+1;
    //if (!(tx=mallocsafe(txsz,"main"))) exit(1);
    //strcpy(tx," ");
    //strncpy(tx,sts,txsz);
    //destlen=strlen(tx);
    //memstr(tx,tx+destlen,"Hello",5);
    if (txsz<1189) txsz=1189;
    strcpyarmsafe(&tx,&txsz,&destlen,sts,"main");
#if 0
    puts(strtokX(tx,&i,", "));
    puts(strtokX(tx,&i,", "));
    puts(strtokX(tx,&i,", "));
    puts(strtokX(tx,&i,", "));
    puts(strtokX(tx,&i,", "));
#endif
    //funcmain(NULL,NULL);

#if 1 /* 1=process all, 0=process line by line */
  /*for(i=8; i<=254; i++) {
    destlen=0; strcpyarmsafe(&tx,&txsz,&destlen,sts,"main");
    rvu=rewraptexttest(&tx,&txsz,&destlen,i,0);
    if (!tx) break;
  }
  if (tx) {
    //rewraptextcatest(&tx,&txsz,&destlen,0,0);
    printf("Done %u\n",rvu);
#if NPPDEBUG
    printout(tx,"\n");
    printf("All OK up to %d!\n",i);
#else
    printf("All OK up to %d!\n%s\n",i,st);
#endif
  }*/

  //rvu=strtran(st,&sln,NULL,NULL,"'","\\\"",&maxexpand,&fail);
  //rvu=strmstr(st,&rvc,mstring,NELEM(mstring),&nm,1); printf("Done %d\n%s\n",rv,rvc);
  //rvu=findqtstringsca(st,&sln,",",1,&maxexpand,&fail);
  //rvu=prepostpend(st,&sln,0,"echo \"","\\n\";","\\\"",";\\\\\\;\"\\\";",&maxexpand,&fail);
  //rvu=insertselectioncolumnca(st,&sln,"$$M$soft",0,&maxexpand,&fail); //"$-8$#16+10"
  //rvu=0; strcpy(st,""); printf("%u-%d %u-%d %u-%d %u-%d %u-%d\n",56,powcheck(56),32,powcheck(32),1,powcheck(1),0,powcheck(0),32768u,powcheck(32768u));
  //rvu=addup(st);
  //rvu=space2tabsca(st,&sln,0,2,0,&maxexpand,&fail);
  //rvu=strqsortlines(st,&sln,1,4,&maxexpand,&fail);
  //rvu=trimtrailingspaceca(st,&sln,&maxexpand,&fail);
  //rvu=c_astyle2(st,&sln,0,2,&maxexpand,&fail);

  //rvu=stripHTMLtags(&tx,&txsz,&destlen,"n");
  //rvu=memchrt(&tx,&txsz,&destlen,"cas","@#$");
  //rvu=deleteeverynthline(tx,txsz,&destlen,2);
  //rvu=lineup(&tx,&txsz,&destlen,';',1,8,1);
  //rvu=encodeURIcomponent(&tx,&txsz,&destlen);
  //rvu=strchrt(&tx,&txsz,&destlen,"case","esa");
  //rvu=filldown(&tx,&txsz,&destlen,1);
  //rvu=tohex(&tx,&txsz,&destlen,6551); rvu=fromhex(&tx,&txsz,&destlen,6551);   if (strcmp(tx,sts)) puts("Miscompare");
  //rvu=extendblockspaces(&tx,&txsz,&destlen);
  //rvu=base64decode(&tx,&txsz,&destlen);
  //rvu=hexbyterunstolittlendian(&tx,&txsz,&destlen,2);
  //rvu=littlendiantohexbyteruns(&tx,&txsz,&destlen);
  //rvu=indentlinessurround(&tx,&txsz,&destlen,0,2,0);
  //rvu=splitlinesatch(&tx,&txsz,&destlen,',',0,0);
  //rvu=strchrstrans(&tx,&txsz,&destlen,NULL,"&<>\"",4,",&&amp;,<&lt;,>&gt;,\"&quot;,");

  //rvc=smprintfpath("%u%?7%xC%C%?:%s%?\\%s%?\\%s",65,128,'C',"\\WINDOWS\\","TEMP","TEMPFILE.TXT");
  //destlen=sarmprintfpath(&tx,&txsz,NULL,"%d + %d = %d\r\n",5,7,5+7);
  //rvu=sarmprintfpath(&tx,&txsz,&destlen,"%#5#$%#**%u%?7%xC%C%?:%s%?\\%s%?\\%s",11,'!',65,128,'C',"\\WINDOWS\\","TEMP","TEMPFILE.TXT");
  //rvc=smprintf("%c%d%f",'x',59,33.26);
  //snprintfX(st,10+0*sizeof(st),"%c%d%f",'x',59,33.26);

  //free(tx); tx=NULL;
  //strcpyarm(&tx,&txsz,&destlen,"");
  //destlen=strcpyarm(&tx,&txsz,NULL,"ZZZ");
  //destlen=strcpyarm(&tx,&txsz,NULL,"AAA");
  //strcpyarm(&tx,&txsz,&destlen,"0123456789A");
  //strcpyarm(&tx,&txsz,&destlen,"");
  //strcpyarm(&tx,&txsz,&destlen,"A");
  //strcpyarm(&tx,&txsz,&destlen,"How Are You");
  //destlen=strcpyarm(&tx,&txsz,NULL,"Everything is fine here");
  //sarmprintf(&tx,&txsz,&destlen,"%d + %21d = %d",4,3,4+3);
  //strcpyarm(&tx,&txsz,&destlen,", ");
  //sarmprintf(&tx,&txsz,&destlen,"%d * %d = %d",6,2,6*2);
  //sarmprintf(&tx,&txsz,&destlen,"%d^2 + %d^2 = %d^2",3,4,5);
  //if (fail) free(fail);

#else
  for(i=1,rv=0,rvc=st; *rvc && !fail; i++) {
    printf("\nLine %d ",i);
    //sln=0; //sln+=7;
    rv+=strchrstrans(st,&sln,&rvc,"\r\n&<>\"",",&&amp;,<&lt;,>&gt;,\"&quot;,",&maxexpand,&fail);
    //rv+=strtran(st,&sln,&rvc,"\"","\\\"",&maxexpand,&fail);
    while(*rvc=='\r' || *rvc=='\n') rvc++;
  }
#endif
  printf("Done(sequence:%d - %s) rvu:%d\n%s\n",i,!tx?"Fail":"Success",rvu,rvc?rvc:tx);
  if (tx) {
    if (strlen(tx)!=destlen) {
      printf("destlen not calculated properly strlen(tx):%u != destlen:%u\n",strlen(tx),destlen);
      destlen=strlen(tx);
    }
    freesafe(tx,"main");
  }
  /*if (spc) {
    free(spc);
    spc=NULL;
    spclen=0;
  }*/
#ifdef mallocsafeinit
  mallocsafedone();
#endif
  return(0);
}
