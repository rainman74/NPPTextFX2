// DISCARD.CPP, old code that did something useful but ended up not being needed.

#define IMPORT_ASTYLE 0 /* the code was too poor quality */
//#include <iostream>
//#include <sstream>
#include <string>
#include "astyle/astyle.h"
//using namespace std;
//using namespace astyle;
// From astyle_main.cpp
class ASStreamIterator : public astyle::ASSourceIterator {
    public:
        ASStreamIterator(char *cin);
        virtual ~ASStreamIterator();
        bool hasMoreLines() const;
        string nextLine();
        char lastcrlf[3];
        unsigned lastcrlflen;

    private:
        char *inString;
};

ASStreamIterator::ASStreamIterator(char *in)    { inString = in; *lastcrlf='\0'; lastcrlflen=0;}
ASStreamIterator::~ASStreamIterator()           { /*delete inStream;*/}
bool ASStreamIterator::hasMoreLines() const     { return((*inString)?true:false);}

// This iterator produces a c-string a line at a time
string ASStreamIterator::nextLine() {
  int chn=strcspn(inString,"\r\n");
  char tempch=inString[chn];
  inString[chn]='\0';
  string str=string(inString);
  inString[chn]=tempch;
  inString+=chn;
  if (!*inString) lastcrlflen=0;
  else lastcrlflen=(*inString=='\r' && inString[1]=='\n')?2:1;
  if (lastcrlflen) memcpy(lastcrlf,inString,lastcrlflen);
  lastcrlf[lastcrlflen]='\0';
  inString+=lastcrlflen;
  return(str);
}

unsigned c_astyle(char *str,unsigned *strsln,unsigned *maxexpand,char **fail) {
  unsigned nv=0,sln;
  if (!fail || !*fail) {
    GetSLNValue("c_astyle")
    char *nstr; if (nstr=strdup(str)) {
      ::ASStreamIterator ite(nstr);

      //FormatterSettings settings;
      //settings.ApplyTo(formatter);

     *str='\0';
     if (maxexpand) {*maxexpand += sln; sln=0; }
#if 0
      while(ite.hasMoreLines()) {
        strcat(str,ite.nextLine().c_str());
        strcat(str,ite.lastcrlf);
      }
#else
      astyle::ASFormatter formatter;
      formatter.setCStyle();
      formatter.init(&ite);
      while (formatter.hasMoreLines()) {
        strcat(str,formatter.nextLine().c_str());
        strcat(str,ite.lastcrlf);
      }
#endif
      free(nstr);
      sln=strlen(str);
      if (strsln) *strsln = sln;
      if (maxexpand) *maxexpand -= sln;
      nv=1;
    }
  }
  return(nv);
}
#endif


// http://en.wikipedia.org/wiki/Binary_search
// array must be numeric only, continuous aka no sparseness, and sorted ascending
// returns key of found item or -1 if not found
// different bsearch algorithms are able to target the lowest or highest of a run of equals but we don't need that here
void *bsearchX(void *key,void *base,size_t nelem,size_t width,int (*fcmp)(const void *, const void *)) {
  void *targ;
#if 1
  size_t n;
  for(targ=base, n=0; n<nelem; (char *)targ+=width, n++) if (!fcmp(key,targ)) return(targ);
  return(NULL);
#else
  size_t left=0,right=nelem-1,midd;
  int dif;

  while (right-left>2) {
    midd = (left+right)/2;
    targ=(char *)base+midd*width;
    dif=fcmp(key,targ);
    if (dif<0) right=midd-1;
    else left=midd;
    //else if (dif>0) left=midd+1;
    //else return targ;
  }
  for(targ=(char *)base+left*width; left<=right; left++, (char *)targ+=width) if (!fcmp(key,targ)) return(targ);
  return NULL;
#endif
}

char *smprintf(size_t *size,const char *format,...) {
  char *rv;
  va_list ap;
  size_t len,vs;
  va_start(ap, format);
#if NPPDEBUG /* { */
  len=8;  /* initial buffer size */
#else /* } else { */
  len=1024;
#endif /* } */
  for(rv=(char *)malloc(len/*+100*/); len && rv && !(rv[len-1]='\0') && ((vs=vsnprintf(rv,len-1,format,ap))>len-1 || rv[len-1]); free(rv),rv=(char *)malloc(len)) {
#ifdef __WATCOMC__ /* { apparently you can't use the va_list over and over in Watcom-C */
  	va_end(ap);
  	va_start(ap, format);
  	vs++; /* when the buffer is too short, Watcom-C returns the entire required buffer length including the \0 byte, +1 from everyone else's library */
#endif /* } */
#if defined(__MINGW32__) || defined(_MSC_VER) || defined(__DMC__) /* {  bugged library returns -1 not enough buffer space */
  	if (vs == (unsigned)-1) len *=2; else
#endif /* } */
  	len=vs+1; /* if snprintf() ever returned -1 for any other reason, len would become 0 which would cause a halt */
#if NPPWINDOWS /* { no recursive calls please */
    //char debug[256]; snprintfX(debug,sizeof(debug),"vsnprintf() returns %u; Trying length %u",vs,len); MessageBox(nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
  }
  va_end(ap);
#if NPPDEBUG /* { */
  if (vs != strlen(rv)) { /* no recursive calls please */
    char debug[256]; snprintfX(debug,sizeof(debug),"vs=%u strlen(rv)=%u",vs,strlen(rv)); MessageBox(nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
    vs = strlen(rv);
  }
#endif /* } */
  if (rv && vs+1<len) {
    rv=(char *)realloc(rv,vs+1);
#if NPPWINDOWS /* { no recursive calls please */
    //char debug[256]; snprintfX(debug,sizeof(debug),"Final length (including \\0) was %u",vs+1); MessageBox(nppData._nppHandle,debug,PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
  }
  if (size) *size=rv?vs+1:0;
  //if (rv) puts(rv);
  return(rv);
}

#if 0 /* { nice code but so far always solved in another better way, also not rentrant */
static char *spc=NULL; // spc should be free() at the program end if not NULL
static unsigned spclen=0;
char *replicate(unsigned n,char ch) {
  if (!n || !ch) return("");
  if (spclen<n || !spc) {
    if (spc) freesafe(spc,"replicate");
    if (!(spc=(char *)mallocsafe(n+1,"replicate"))) {
      spclen=0;
      return("");
    }
    spc[0]='\0';
    spclen=n;
  }
  if (spc[0]!=ch) {
    memset(spc,ch,spclen);
    spc[spclen]='\0';
  }
  return(spc+spclen-n);
}
#endif /* } */

#define IsDialogEx(pTemplate) (((DLGTEMPLATEEX*)pTemplate)->signature == (WORD)-1)
#define DialogSkipNameOrdinal(pb) ((*((WORD *)pb) == (WORD)-1)?(2*sizeof(WORD)):(sizeof(WCHAR)*(wcslen((WCHAR *)pb)+1)))
typedef struct { // Microsoft Platform SDK\src\mfc\afximpl.h
	WORD dlgVer,signature;
	DWORD helpID,exStyle,style;
	WORD cDlgItems;
	short x,y,cx,cy;
} DLGTEMPLATEEX;

typedef struct { // Microsoft Platform SDK\src\mfc\afximpl.h
	DWORD helpID,exStyle,style;
	short x,y,cx,cy;
	DWORD id;
} DLGITEMTEMPLATEEX;

#if 0
/* Resources aren't really GlobalAlloc() memory any more and it's unreasonably difficult to detect the
   actual size of a dialog. We'll just feel the memory with IsBadReadPtr() and if we get too much, who cares! */
EXTERNC DWORD MemAllocMaxSize(LPCVOID pVoid) {
  DWORD dwMin,dwMax,dwMid;
  for(dwMin=0,dwMax=1; !IsBadReadPtr(pVoid,dwMax); dwMin=dwMax,dwMax*=2);
  while(1) {
    dwMid=(dwMax+dwMin)/2;
    if (dwMid==dwMin) break;
    else if (IsBadReadPtr(pVoid,dwMid)) dwMax=dwMid;
    else dwMin=dwMid;
  }
  return(dwMin);
}
#define GetTemplateSize MemAllocMaxSize
#else
 // Skip menu name string or ordinal
EXTERNC DWORD GetTemplateSize(const DLGTEMPLATE *pTemplate) { // Microsoft Platform SDK\src\mfc\dlgtempl.cpp
	BYTE* pb;
	WORD nCtrl;
	BOOL bDialogEx = IsDialogEx(pTemplate);

	pb = bDialogEx?((BYTE*)((DLGTEMPLATEEX*)pTemplate + 1)):((BYTE*)(pTemplate + 1)); // Skip header
	pb += DialogSkipNameOrdinal(pb);  // Skip menu name string or ordinal
	pb += DialogSkipNameOrdinal(pb);  // Skip class name string or ordinal
	pb += sizeof(WCHAR)*(wcslen((WCHAR *)pb)+1);       // Skip caption string
	if (DS_SETFONT & (bDialogEx ? ((DLGTEMPLATEEX*)pTemplate)->style:pTemplate->style)) {
		pb += (bDialogEx?(sizeof(WORD)*3):(sizeof(WORD)*1));  // Skip font size, weight, (italic, charset)
		pb += sizeof(WCHAR)*(wcslen((WCHAR*)pb) + 1); // Skip font name
	}
	for(nCtrl = bDialogEx ? (WORD)((DLGTEMPLATEEX*)pTemplate)->cDlgItems :(WORD)pTemplate->cdit; nCtrl ; --nCtrl) {
		pb = (BYTE*)(((DWORD_PTR)(pb+3)) & ~((DWORD_PTR)3)); // DWORD align
		pb += (bDialogEx ? sizeof(DLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE));
   	pb += DialogSkipNameOrdinal(pb);  // Skip class name string or ordinal
    pb += DialogSkipNameOrdinal(pb); // Skip text name string or ordinal
		pb += sizeof(WORD) + (*((WORD*)pb)); // Skip extra data
	}
	return (DWORD)(pb - (BYTE*)pTemplate); //IA64: Max dialog template size of 4GB should be fine
}
#endif

EXTERNC void GetWindowTextUTF8arm(HWND hwnd,char **ppBufA,size_t *puBufsz,size_t *puBufLenA,BOOL fUnicode,unsigned eoltype) {
  if (fUnicode) {
    size_t uBufSzW=GetWindowTextLengthW(hwnd)+1; // we need the null byte
    wchar_t *pBufW=(wchar_t *)mallocsafe(uBufSzW*sizeof(wchar_t),"strdupGetWindowText"); if (!pBufW) return;
    uBufSzW=GetWindowTextW(hwnd,pBufW,uBufSzW)+1; // we need the null byte
    size_t uBufLenA=UTF8FromUCS2(pBufW,uBufSzW,NULL,0,0);
    armreallocsafe(ppBufA,puBufsz,uBufLenA,ARMSTRATEGY_MAINTAIN,0,"strdupGetWindowText");
    if (*ppBufA) {
      *puBufLenA=UTF8FromUCS2(pBufW,uBufSzW,*ppBufA,uBufLenA,0)-1; //MessageBoxFree(hwnd,smprintf("uBufSzW:%u uBufLenA:%u text:%s",uBufSzW,uBufLenA,pBufA),"???", MB_OK|MB_ICONSTOP);
    }
    freesafe(pBufW,"strdupGetWindowText");
  } else {
    size_t uBufLenA=GetWindowTextLengthA(hwnd)+1;
    armreallocsafe(ppBufA,puBufsz,uBufLenA,ARMSTRATEGY_MAINTAIN,0,"strdupGetWindowText");
    if (*ppBufA) *puBufLenA=GetWindowTextA(hwnd,*ppBufA,uBufLenA);
  }
  if (eoltype != SC_EOL_CRLF) converteol(ppBufA,puBufsz,puBufLenA,eoltype); // the "edit" class only provides CRLF
  //return pBufA;
}

EXTERNC BOOL SetWindowTextUTF8(HWND hwnd,char **ppBufA,size_t *puBufSzA,size_t *puBufLenA,BOOL fUnicode,unsigned eoltype) {
  BOOL rv=FALSE;
  converteol(ppBufA,puBufSzA,puBufLenA,SC_EOL_CRLF); // two converteol()'s are required and this is the easiest arrangement
  if (*ppBufA && *puBufLenA) {
    if (fUnicode) {
      size_t uBufSzW=UCS2FromUTF8(*ppBufA,*puBufLenA+1,NULL,0,FALSE,NULL);
      wchar_t *pBufW=(wchar_t *)mallocsafe(uBufSzW*sizeof(wchar_t),"SetWindowTextUTF8");
      if (pBufW) {
        UCS2FromUTF8(*ppBufA,*puBufLenA+1,pBufW,uBufSzW,FALSE,NULL);
        rv=SetWindowTextW(hwnd,pBufW);
        freesafe(pBufW,"SetWindowTextUTF8");
      }
    } else {
      rv=SetWindowTextA(hwnd,*ppBufA);
    }
    if (eoltype != SC_EOL_CRLF) converteol(ppBufA,puBufSzA,puBufLenA,eoltype);
  }
  return rv;
}

**Demo: Improve Home-End ...

Many people ask for Home and End to not go from BOL to EOL in line wrap mode. The ability to go EOL and BOL of screen lines is built in to Scintilla. This option enables it.

Code folding is a poor attempt at showing the programmer what if, while, or function a far out } belongs to. I just want to see the matching line and I can do that by placing the cursor after the } in the struct above. Select Show line matching Brace and the N++ title bar will be set to the line matching the {. Note that this only works backwards. You can't sit on the { brace and ask for the last line because I can't think of any reason why that would be useful. At this time "} else {" lines are not processed properly but this proof of concept will let me guage how valuable a feature like this is to programmers. 

