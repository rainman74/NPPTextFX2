//#define PLUGIN_NAME "TextFX"
//#define SUPPORT_PATH "NPPTextFX" /* thanks to smprintfpath we don't need to put a slash on the end of this */

#ifdef _MSC_VER
#define vsnprintf _vsnprintf /* why doesn't Microsoft provide this now standard function? */
#endif
#if NPPDEBUG
#if (defined(__DMC__) || defined(_MSC_VER))
#define snprintf _snprintf /* snprintf is unsafe so we only want to use our snprintfX() */
#endif
#else
#define snprintf snprintf_unsafe
#endif
#define sprintf sprintf_unsafe
#define strncpy strncpy_unsafe

// Not the best way to solve this, but it works. Use this instead of size_t where previously 32-bit pointers have been used.
typedef unsigned int tsize_t;

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

#ifndef NELEM
#define NELEM(xyzzy) (sizeof(xyzzy)/sizeof(xyzzy[0]))
#endif

#ifndef NPPDEBUG
#error NPPDEBUG Must be defined as a -D option for all source files
#endif

EXTERNC void *ReallocFree(void *memblock, size_t size);
#if NPPDEBUG
EXTERNC void *freesafebounds(void *bf,unsigned ct,const char *title);
EXTERNC void *reallocsafebounds(void *bf,unsigned ct);
EXTERNC void freesafe(void *bf,const char *title);
EXTERNC void *reallocsafeX(void *bf,unsigned ct,char *title,int allownull);

/* This will only be set by source code that promises to do this later */
#ifndef NPPTextFX_DELAYUNSAFE
#define mallocsafe(ct,ti) reallocsafeX(NULL,ct,ti,1)
#define reallocsafe(bf,ct,ti) reallocsafeX(bf,ct,ti,0)
#define reallocsafeNULL(bf,ct,ti) reallocsafeX(bf,ct,ti,1)
#define malloc malloc_unsafe
#define realloc realloc_unsafe
#define calloc calloc_unsafe /* I never use calloc() but just in case someone tries */
#define free free_unsafe
#define strdup strdup_unsafe
#endif

EXTERNC char *strdupsafe(const char *source,char *title);
#else
#define mallocsafe(ct,ti) malloc(ct)
#define reallocsafe(bf,ct,ti) ReallocFree(bf,ct)
#define reallocsafeNULL(bf,ct,ti) ReallocFree(bf,ct)
#define freesafe(bf,ti) free(bf)
#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define strdupsafe(bf,ti) _strdup(bf)
#else
#define strdupsafe(bf,ti) strdup(bf)
#endif
#endif

#if NPPDEBUG
EXTERNC char *memdupsafe(const char *source,unsigned ls,char *title);
#else
#define memdupsafe(bf,ln,ti) memdup(bf,ln)
EXTERNC char *memdup(const char *source,unsigned ls);
#endif

#if NPPDEBUG
EXTERNC char *memdupzsafe(const char *source,unsigned ls,char *title);
#else
#define memdupzsafe(bf,ln,ti) memdupz(bf,ln)
EXTERNC char *memdupz(const char *source,unsigned ls);
#endif

EXTERNC size_t roundtonextpower(size_t numo); /* round to next power */

#define ARMSTRATEGY_INCREASE 0 //increase buffer with slack space when necessary; good for a constantly expanding buffer
#define ARMSTRATEGY_MAINTAIN 1 //increase buffer only the minimum amount, buffer will not be reduced if too large; good for a buffer that will be reused with some large and some small allocations
#define ARMSTRATEGY_REDUCE 2   //increase buffer only the minimum amount, reduce buffer if too large; good for buffers of known size or that will only be used once
EXTERNC int armrealloc(char **dest,unsigned *destsz,unsigned newsize,int strategy,int clear
#undef THETITLE
#if NPPDEBUG
,char *title
#define armreallocsafe armrealloc
#define THETITLE title
#else
#define armreallocsafe(dt,ds,ns,st,cl,ti) armrealloc(dt,ds,ns,st,cl)
#define THETITLE "armrealloc" /* the macros discards this */
#endif
);

EXTERNC int strncpyarm(char **dest,tsize_t *destsz,tsize_t *destlen,const char *source,size_t maxlen
#undef THETITLE
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
);

EXTERNC int memcpyarm(char **dest,tsize_t *destsz,tsize_t *destlen,const char *source,size_t slen
#undef THETITLE
#if NPPDEBUG
,char *title
#define memcpyarmsafe memcpyarm
#define THETITLE title
#else
#define memcpyarmsafe(dt,ds,dl,st,ml,ti) memcpyarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
);

EXTERNC int memsetarm(char **dest,tsize_t *destsz,tsize_t *destlen,int chr,size_t slen
#undef THETITLE
#if NPPDEBUG
,char *title
#define memsetarmsafe memsetarm
#define THETITLE title
#else
#define memsetarmsafe(dt,ds,dl,st,ml,ti) memsetarm(dt,ds,dl,st,ml)
#define THETITLE "memcpyarm" /* the macros discards this */
#endif
);
#undef THETITLE

EXTERNC size_t snprintfX(char *buffer,tsize_t buffersz,const char *format,...);
EXTERNC size_t vsarmprintf(char **dest,tsize_t *destsz,tsize_t *destlen,const char *format,va_list ap2);
EXTERNC size_t sarmprintf(char **dest,tsize_t *destsz,tsize_t *destlen,const char *format,...);
EXTERNC char *smprintf(const char *format,...);
EXTERNC size_t vsarmprintfpath(char **dest,unsigned *destsz,unsigned *destlen,const char *format,va_list ap2);
EXTERNC size_t sarmprintfpath(char **dest,tsize_t *destsz,tsize_t *destlen,const char *format,...);
EXTERNC char *smprintfpath(const char *format,...);
EXTERNC int MessageBoxFree(HWND hWnd,TCHAR *lpText,LPCTSTR lpCaption,UINT uType);
EXTERNC int memmovearm(char **dest,unsigned *destsz,unsigned *destlen,char *destp,char *sourcep
#if NPPDEBUG
,int notest
#endif
);

#if NPPDEBUG
#define memmovearmtest memmovearm
#else
#define memmovearmtest(dt,ds,dl,dp,sp,nt) memmovearm(dt,ds,dl,dp,sp)
#endif

EXTERNC void memcqspnstart(const char *find,unsigned findl,unsigned *quick);
EXTERNC char *memcqspn(const char *buf,const char *end,const unsigned *quick);
EXTERNC char *memqspn(const char *buf,const char *end,const unsigned *quick);
EXTERNC char *memcspn(const char *buf,const char *end,const char *find,unsigned findl);
EXTERNC char *memspn(const char *buf,const char *end,const char *find,unsigned findl);
EXTERNC char *memstr(const char *buf,const char *end,const char *find,unsigned findl);
EXTERNC char *memchrX(const char *buf,const char *end,unsigned find);
EXTERNC char *strncpymem(char *szDest,size_t uDestSz,const char *sSource,unsigned uSourceLen);
#define strncpy strncpy_unsafe
char *strtokX(char *szStr,unsigned *uPos,const char *szDeli);
#define strtok strtok_unsafe
EXTERNC unsigned powcheck(unsigned num);

EXTERNC unsigned converteol(char **dest,unsigned *destsz,unsigned *destlen,unsigned eoltype);


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
EXTERNC DWORD _writeX(HANDLE handle,const void *buf,DWORD len);
EXTERNC DWORD _readX(HANDLE handle,void *buf,DWORD len);
#define _closeX(hnd) CloseHandle(hnd)

EXTERNC char *findnextquote(char *str,char *end,unsigned style);

EXTERNC DWORD GetPrivateProfileStringarm(LPCTSTR lpAppName,LPCTSTR lpKeyName,LPCTSTR lpDefault,char **dest,unsigned *destsz,LPCTSTR lpFileName);
EXTERNC BOOL WritePrivateProfileStringFree(LPCTSTR lpAppName,LPCTSTR lpKeyName,TCHAR *lpString,LPCTSTR lpFileName);

EXTERNC BOOL isFileExist(const char *fn);

/* examine shlobj.h for a valid range */
#define CSIDLX_TEXTFXDATA         0x00FE // Static Data provided with install
//#define CSIDLX_NOTEPADPLUSEXE     0x00FD
//#define CSIDLX_NOTEPADPLUSPLUGINS 0x00FC
#define CSIDLX_TEXTFXTEMP         0x00FB
#define CSIDLX_TEXTFXINIFILE      0x00FA
#define CSIDLX_USER               0x00FA /* set this to the lowest custom value */

EXTERNC unsigned NPPGetSpecialFolderLocationarm(int nFolder,const char *szName,char **pszFolder,unsigned *puFolderSz,unsigned *puFolderLen,const char *);
EXTERNC BOOL XlatPathEnvVarsarm(char **dest,unsigned *destsz,unsigned *destlen);
