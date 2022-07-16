#define INTPACKEDBOOLEAN(find,array,typ) ((array[(find)/(sizeof(typ)*8)] & 1<<(find)%(sizeof(typ)*8))?TRUE:FALSE)

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

EXTERNC int CapsTablesWStart(unsigned); /* returns 0 on failure */
EXTERNC void CapsTablesWStop(int);

#if defined(NPPDEBUG) && NPPDEBUG
#define BLOCKNT 1 /* Set this to 1 to use Windows 98 functionality, mainly for testing the routines */
#else
#define BLOCKNT 0
#endif

extern BOOL g_fOnNT;

#if 1 /* Destroy these macros to allow them to be functions instead */
#define IsCharUpperXW(wc) ((!BLOCKNT && g_fOnNT)?IsCharUpperW((wchar_t)(wc)):INTPACKEDBOOLEAN((wchar_t)(wc),ISCHARUPPERCTW,unsigned))
#define IsCharLowerXW(wc) ((!BLOCKNT && g_fOnNT)?IsCharLowerW((wchar_t)(wc)):INTPACKEDBOOLEAN((wchar_t)(wc),ISCHARLOWERCTW,unsigned))
#define IsCharAlphaNumericXW(wc) ((!BLOCKNT && g_fOnNT)?IsCharAlphaNumericW((wchar_t)(wc)):INTPACKEDBOOLEAN((wchar_t)(wc),ISCHARALPHANUMERICCTW,unsigned))
#define CharUpperXW(wc) ((!BLOCKNT && g_fOnNT)?((wchar_t)(unsigned)CharUpperW((wchar_t *)(unsigned)(wchar_t)(wc))):CHARUPPERCTW[(wchar_t)(wc)])
#define CharLowerXW(wc) ((!BLOCKNT && g_fOnNT)?((wchar_t)(unsigned)CharLowerW((wchar_t *)(unsigned)(wchar_t)(wc))):CHARLOWERCTW[(wchar_t)(wc)])
extern unsigned *ISCHARUPPERCTW,*ISCHARLOWERCTW,*ISCHARALPHANUMERICCTW;
extern wchar_t *CHARUPPERCTW,*CHARLOWERCTW;
#else
EXTERNC wchar_t CharUpperXW(wchar_t wc);
EXTERNC wchar_t CharLowerXW(wchar_t wc);
EXTERNC BOOL IsCharUpperXW(wchar_t wc);
EXTERNC BOOL IsCharLowerXW(wchar_t wc);
EXTERNC BOOL IsCharAlphaNumericXW(wchar_t wc);
#endif

#define CharUpperXA(chA) ((char)(unsigned char)(unsigned)CharUpperA((char *)(unsigned)(unsigned char)chA))
#define CharLowerXA(chA) ((char)(unsigned char)(unsigned)CharLowerA((char *)(unsigned)(unsigned char)chA))

#ifdef UNICODE
#define CharUpperX CharUpperXW
#define CharLowerX CharLowerXW
#define IsCharUpperX IsCharUpperXW
#define IsCharLowerX IsCharLowerXW
#define IsCharAlphaNumericX IsCharAlphaNumericXW
#else
#define CharUpperX CharUpperXA
#define CharLowerX CharLowerXA
#define IsCharUpperX IsCharUpperA
#define IsCharLowerX IsCharLowerA
#define IsCharAlphaNumericX IsCharAlphaNumericA
#endif
