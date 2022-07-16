/*--------------------------------------------------------
  Based on Petzold's POPFIND.C out of POPPAD3.C
  Though it would be legal to use Petzold's code, I don't think any
  of it still exists.
  --------------------------------------------------------*/

#include /*..*/"enable.h"
#if ENABLE_FINDREPLACE
//#endif
#define _WIN32_IE 0x0500
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <ctype.h>

//#include <shlobj.h>  /* MSVC: needs shlwapi.h copied from MS Visual Studio to work */

#include <stdio.h>
//#include <tchar.h>            // for _tcsstr (strstr for Unicode & non-Unicode)
//#include <stdlib.h>
#include /*..*/"capstables.h"
#include /*..*/"PluginInterface.h"
#include /*..*/"MicroXML/MicroXML.h"

#if defined(__WATCOMC__) || defined(__DMC__) || defined(_MSC_VER)
#define snwprintf _snwprintf
#endif

#include "popfind.h"
#include /*..*/"NPPTextFX.h"
#include /*..*/"Scintilla.h"
#include /*..*/"Scintilla/UniConversion.h"

#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define itoa _itoa
#define memicmp _memicmp
#if defined(__POCC__)
#pragma warn(disable:2216 2209) /* Return value from function is never used; Unreachable code removed */
#endif
#endif

#ifdef __BORLANDC__
#pragma warn -pia /* Borland-C: possibly incorrect assignment */
#pragma warn -ccc       /* Borland-C: condition is always true/false */
#pragma warn -rch       /* Borland-C: Unreachable code */
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL g_fOnNT;
//extern HINSTANCE g_hInstance;

#if NPPDEBUG && 0 /* 1=test Win9x functions, 0=normal usage */
#define g_fISNT FALSE
#else
#define g_fISNT g_fOnNT
#endif

#define SIGNEDMAXPOSITIVE (((unsigned)-1)>>1)
#define SIGNEDMAXNEGATIVE (((unsigned)-1)>>1+1)

static const char *g_tszRegex[]={
  "Select Regex",
  "Match Any Character: .",
  "Tag Region Start: (", // POSIX removes the \\ before ( and )
  "Tag Region End: )",
  "Word Start: \\<",
  "Word End: \\>",
  "Character Literal: \\",
  "Set of Characters: []",
  "Complementary Set: [^]",
  "Line Start: ^",
  "Line End: $",
  "Match 0+ times: *",
  "Match 1+ times: +",
  "Replace Tag Region: \\1"
};

static char g_szTitle[MAX_PATH],g_szFile[MAX_PATH],g_szLastLoadedFile[MAX_PATH],g_szInit[MAX_PATH];
static OPENFILENAME g_ofn={
     /*lStructSize       =*/ sizeof (OPENFILENAME),
     /*hwndOwner         =*/ NULL , // Set in Open and Close functions
     /*hInstance         =*/ NULL ,
     /*lpstrFilter       =*/ TEXT ("Find & Replace Files (*.FRXML)\0*.frxml\0")
                             TEXT ("All Files (*.*)\0*.*\0\0"),
     /*lpstrCustomFilter =*/ NULL ,
     /*nMaxCustFilter    =*/ 0 ,
     /*nFilterIndex      =*/ 0 ,
     /*lpstrFile         =*/ g_szFile ,
     /*nMaxFile          =*/ NELEM(g_szFile) ,
     /*lpstrFileTitle    =*/ g_szTitle ,
     /*nMaxFileTitle     =*/ NELEM(g_szTitle) ,
     /*lpstrInitialDir   =*/ g_szInit ,
     /*lpstrTitle        =*/ NULL ,
     /*Flags             =*/ 0 ,             // Set in Open and Close functions
     /*nFileOffset       =*/ 0 ,
     /*nFileExtension    =*/ 0 ,
     /*lpstrDefExt       =*/ TEXT ("frxml") ,
     /*lCustData         =*/ 0 ,
     /*lpfnHook          =*/ NULL ,
     /*lpTemplateName    =*/ NULL
};

static void CheckFlags(HWND hDlg,FINDREPLACEX *pfr) {
  if ((pfr->ScintillaFlags&SCFIND_REGEXP)) {
    EnableWindow(GetDlgItem(hDlg,IDC_RADDOWN),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_RADUP),FALSE);
    //EnableWindow(GetDlgItem(hDlg,IDC_CHXREPLCASE),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_CHXWHOLEWORD),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_CHXSTARTWORD),FALSE);
    pfr->StorageFlags |= pfr->PersistentFlags&(FRP_UP/*|FRP_REPLCASE*/);
    pfr->PersistentFlags &= ~(FRP_UP/*|FRP_REPLCASE*/);
    pfr->SciStorageFlags |= (pfr->ScintillaFlags&(SCFIND_WHOLEWORD|SCFIND_WORDSTART));
    pfr->ScintillaFlags &= ~(SCFIND_WHOLEWORD|SCFIND_WORDSTART);
  } else {
    EnableWindow(GetDlgItem(hDlg,IDC_RADDOWN),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_RADUP),TRUE);
    //EnableWindow(GetDlgItem(hDlg,IDC_CHXREPLCASE),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_CHXWHOLEWORD),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_CHXSTARTWORD),TRUE);
    pfr->PersistentFlags|=(pfr->StorageFlags&(FRP_UP/*|FRP_REPLCASE*/));
    pfr->StorageFlags&=~(FRP_UP/*|FRP_REPLCASE*/);
    pfr->ScintillaFlags |= (pfr->SciStorageFlags&(SCFIND_WHOLEWORD|SCFIND_WORDSTART));
    pfr->SciStorageFlags &= ~(SCFIND_WHOLEWORD|SCFIND_WORDSTART);
  }
  CheckRadioButton(hDlg,IDC_RADUP,IDC_RADDOWN,(pfr->PersistentFlags&FRP_UP)?IDC_RADUP:IDC_RADDOWN);
  //SendDlgItemMessage(hDlg,IDC_CHXREPLCASE,BM_SETCHECK,(pfr->PersistentFlags&FRP_REPLCASE)?BST_CHECKED:BST_UNCHECKED,0);
  SendDlgItemMessage(hDlg,IDC_CHXWHOLEWORD,BM_SETCHECK,(pfr->ScintillaFlags&SCFIND_WHOLEWORD)?BST_CHECKED:BST_UNCHECKED,0);
  SendDlgItemMessage(hDlg,IDC_CHXSTARTWORD,BM_SETCHECK,(pfr->ScintillaFlags&SCFIND_WORDSTART)?BST_CHECKED:BST_UNCHECKED,0);
  if ((pfr->PersistentFlags&(FRP_SELECTED|FRP_INCREMENTAL)) || (pfr->InternalFlags&FRI_BOFEOF)) {
    EnableWindow(GetDlgItem(hDlg,IDC_CHXWRAP),FALSE);
    pfr->StorageFlags |= (pfr->PersistentFlags&FRP_WRAP);
    pfr->PersistentFlags &= ~FRP_WRAP;
  } else {
    EnableWindow(GetDlgItem(hDlg,IDC_CHXWRAP),TRUE);
    pfr->PersistentFlags|= (pfr->StorageFlags&FRP_WRAP);
    pfr->StorageFlags&=~FRP_WRAP;
  }
  SendDlgItemMessage (hDlg,IDC_CHXWRAP,BM_SETCHECK,(pfr->PersistentFlags&FRP_WRAP)?BST_CHECKED:BST_UNCHECKED,0);
  if ((pfr->PersistentFlags&(FRP_LINE1ST)) || pfr->uCol1) {
    EnableWindow(GetDlgItem(hDlg,IDC_CHXRECURSIVE),FALSE);
    pfr->StorageFlags |= (pfr->PersistentFlags&FRP_RECURSIVE);
    pfr->PersistentFlags &= ~FRP_RECURSIVE;
  } else {
    EnableWindow(GetDlgItem(hDlg,IDC_CHXRECURSIVE),TRUE);
    pfr->PersistentFlags|=(pfr->StorageFlags&FRP_RECURSIVE);
    pfr->StorageFlags&=~FRP_RECURSIVE;
  }
  SendDlgItemMessage (hDlg,IDC_CHXRECURSIVE,BM_SETCHECK,(pfr->PersistentFlags&FRP_RECURSIVE)?BST_CHECKED:BST_UNCHECKED,0);
}

static BOOL SetWindowTextFree(HWND hWnd,TCHAR *lpText) {
  BOOL rv=SetWindowText(hWnd,lpText?lpText:"a memory problem occured");
  if (lpText) freesafe(lpText,"SetWindowTextFree");
  return(rv);
}

static void GetWindowTextUTF8arm(HWND hwndEditor,HWND hwndControl,char **ppBufA,size_t *puBufsz,size_t *puBufLenA) {
  if (hwndEditor) {
    int iEolMode=SENDMSGTOED(hwndEditor, SCI_GETEOLMODE, 0, 0);
    SENDMSGTOED(hwndControl, SCI_SETEOLMODE, iEolMode, 0);
    SENDMSGTOED(hwndControl, SCI_CONVERTEOLS, iEolMode, 0);
  }
  tsize_t uBufLenA=SENDMSGTOED(hwndControl, SCI_GETLENGTH, 0, 0)+1, tuBufsz=puBufsz?*puBufsz:0, tuBufLenA=puBufLenA?*puBufLenA:0;
  armreallocsafe(ppBufA,&tuBufsz,tuBufLenA,ARMSTRATEGY_MAINTAIN,0,"strdupGetWindowText");
  if (puBufsz) *puBufsz = tuBufsz;
  if (*ppBufA) *puBufLenA=SENDMSGTOED(hwndControl, SCI_GETTEXT, uBufLenA, *ppBufA);
}

#ifndef SCMOD_NORM
#define SCMOD_NORM 0
#endif
static void SetWindowTextUTF8(FINDREPLACEX *pfr,HWND hwndControl,char *pBufA,size_t uBufLenA,int iCodePage) {
  SENDMSGTOED(hwndControl,SCI_SETUNDOCOLLECTION,FALSE,0);
  SENDMSGTOED(hwndControl,SCI_EMPTYUNDOBUFFER,0,0);
  SENDMSGTOED(hwndControl,SCI_CLEARALL,0,0);
  if (iCodePage != -1) {
    SENDMSGTOED(hwndControl,SCI_SETMODEVENTMASK,(SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT),0);
    SENDMSGTOED(hwndControl,SCI_SETCODEPAGE,iCodePage,0);
    if (iCodePage==SC_CP_UTF8) pfr->InternalFlags|=FRI_UNICODE; else pfr->InternalFlags&=~FRI_UNICODE;
    SENDMSGTOED(hwndControl,SCI_CLEARCMDKEY, MAKEWPARAM(SCK_RETURN,SCMOD_NORM) , SCI_NULL); /* These will be handled by subclassing */
    SENDMSGTOED(hwndControl,SCI_CLEARCMDKEY, MAKEWPARAM(SCK_TAB,SCMOD_NORM) , SCI_NULL);
    SENDMSGTOED(hwndControl,SCI_CLEARCMDKEY, MAKEWPARAM(SCK_TAB,SCMOD_SHIFT) , SCI_NULL);
  }
  if (pBufA && uBufLenA) SENDMSGTOED(hwndControl,SCI_APPENDTEXT,uBufLenA,pBufA);
  SENDMSGTOED(hwndControl,SCI_GOTOPOS,0,0);
  SENDMSGTOED(hwndControl,SCI_SETSAVEPOINT,0,0);
  SENDMSGTOED(hwndControl,SCI_SETUNDOCOLLECTION,TRUE,0);
}

#if 1
#define EnableReplaceButtons(x,y)
#else
inline void EnableReplaceButtons(HWND hDlg,BOOL twl) {
  EnableWindow(GetDlgItem(hDlg,IDC_PSHSWAP),twl);
}
#endif

// The dropdowns flicker when you repeatedly set them to 0 when they are already 0
// Use this anytime CURSEL may already be right.
inline void SetCurselOptional(HWND hwndCB,int iIndex) {
  if (SENDMSGTOED(hwndCB,CB_GETCURSEL,0,0) != iIndex) {
    SENDMSGTOED(hwndCB,CB_SETCURSEL,iIndex,0);
  }
}

#define FHFLAG_UNICODE FRI_UNICODE
static struct _FINDHIST {
  unsigned uCount; /* Count of buffers & lengths */
  char **pszBuf; /* List of buffers, always zero terminated 1+ length */
  unsigned *puBufLen; /* List of buffer lengths */
  USHORT *fFlags; /* List of flags */
} g_FindHist,g_ReplHist;

#define HISTORYLIMIT 30
static void HistoryLoadStrings(struct _FINDHIST *pHist,HWND hwndCB,LPCSTR szTitle) {
  COMBOBOXEXITEMA cbiA;
  memset(&cbiA,0,sizeof(cbiA));
  //ShowWindow(hwndCB,SW_HIDE); // WM_PAINT once instead of HISTORYLIMIT times
  SENDMSGTOED(hwndCB,WM_SETREDRAW,FALSE,0);
  SENDMSGTOED(hwndCB,CB_RESETCONTENT,0,0);
  char *szTitleEx=smprintf("%s history",szTitle);
  if (szTitleEx) {
    if (g_fISNT) {
      cbiA.mask=CBEIF_TEXT;
      cbiA.iItem=-1;
      cbiA.pszText=szTitleEx;
      SENDMSGTOED(hwndCB,CBEM_INSERTITEM,0,&cbiA);
    } else SENDMSGTOED(hwndCB,CB_ADDSTRING,0,szTitleEx);
    freesafe(szTitleEx,"HistoryLoadStrings");
  }
  if (pHist->uCount) {
    unsigned i; for(i=0; i<pHist->uCount; i++) {
      if (g_fISNT && (pHist->fFlags[i]&FHFLAG_UNICODE)) {
        COMBOBOXEXITEMW cbiW;
        memset(&cbiW,0,sizeof(cbiW));
        wchar_t szBufW[24],szBufW2[24];
        UCS2FromUTF8(pHist->pszBuf[i],pHist->puBufLen[i],szBufW2,NELEM(szBufW2),FALSE,NULL);
        szBufW2[NELEM(szBufW2)-1]=L'\0';
        snwprintf(szBufW,NELEM(szBufW),L"%2uw: %s",i+1,szBufW2);
        cbiW.mask=CBEIF_TEXT;
        cbiW.iItem=-1;
        cbiW.pszText=szBufW;
        SENDMSGTOED(hwndCB,CBEM_INSERTITEMW,0,&cbiW);
      } else {
        char szBuf[24];
        if (g_fISNT) {
          snprintfX(szBuf,sizeof(szBuf),"%2ua: %s",i+1,pHist->pszBuf[i]);
          cbiA.mask=CBEIF_TEXT;
          cbiA.iItem=-1;
          cbiA.pszText=szBuf;
          SENDMSGTOED(hwndCB,CBEM_INSERTITEMA,0,&cbiA);
        } else {
          snprintfX(szBuf,sizeof(szBuf),"%2u: %s",i+1,pHist->pszBuf[i]);
          SENDMSGTOED(hwndCB,CB_ADDSTRING,0,szBuf);
        }
      }
    }
  }
  SetCurselOptional(hwndCB,0);
  //ShowWindow(hwndCB,SW_SHOW);
  SENDMSGTOED(hwndCB,WM_SETREDRAW,TRUE,0);
  InvalidateRect(hwndCB,NULL,FALSE);
}

static void RepositoryLoadStrings(FINDREPLACEX *pfr,HWND hwndCB) {
  COMBOBOXEXITEMA cbiA;
  memset(&cbiA,0,sizeof(cbiA));
  //ShowWindow(hwndCB,SW_HIDE); // WM_PAINT once instead of HISTORYLIMIT times
  SENDMSGTOED(hwndCB,WM_SETREDRAW,FALSE,0);
  SENDMSGTOED(hwndCB,CB_RESETCONTENT,0,0);
  unsigned uCt=0;
  if (pfr->uPathListLen) {
    char *psTail=pfr->szPathList,*psEnd=psTail+pfr->uPathListLen,*psHead; psTail--;
    do {
      psTail=memchrX(psHead=psTail+1,psEnd,';');
      if (*psTail) {
        *psTail='\0';
        SENDMSGTOED(hwndCB,CB_ADDSTRING,0,psHead);
        *psTail=';';
      } else SENDMSGTOED(hwndCB,CB_ADDSTRING,0,psHead);
      uCt++;
    } while(psTail<psEnd);
  }
  if (uCt && pfr->uPathListSelected>=uCt) pfr->uPathListSelected=uCt-1;
  SetCurselOptional(hwndCB,pfr->uPathListSelected);
  //ShowWindow(hwndCB,SW_SHOW);
  SENDMSGTOED(hwndCB,WM_SETREDRAW,TRUE,0);
  InvalidateRect(hwndCB,NULL,FALSE);
  //PostMessage(pfr->hwndSelf,WM_COMMAND, MAKEWPARAM(IDC_CMBFOLDERS,CBN_SELENDOK) , (LPARAM)hwndCB);
}

static void HistoryInit(struct _FINDHIST *pHist,BOOL stop) {
  if (stop) {
    if (pHist->pszBuf) {
      if (pHist->uCount) {
        unsigned i; for(i=pHist->uCount; i--; ) freesafe(pHist->pszBuf[i],"HistoryInit");
      }
      freesafe(pHist->pszBuf,"HistoryInit");
    }
    if (pHist->puBufLen) freesafe(pHist->puBufLen,"HistoryInit");
    if (pHist->fFlags) freesafe(pHist->fFlags,"HistoryInit");
  }
  memset(pHist,0,sizeof(*pHist));
}

// uBufLen includes a \0
static void HistoryAddString(struct _FINDHIST *pHist,HWND hwndCB,char *sBuf,unsigned uBufLen,USHORT fFlags,LPCSTR szTitle) {
  if (uBufLen>1) {
    unsigned uCount=pHist->uCount;
    if (uCount) {
      unsigned i; for(i=0; i<uCount; i++) if (fFlags == pHist->fFlags[i] && uBufLen == pHist->puBufLen[i] && !memicmp(pHist->pszBuf[i],sBuf,uBufLen)) {
        memcpy(pHist->pszBuf[i],sBuf,uBufLen); // Borland C++ 3.1 DOS forgot this!
        uCount=i;
        goto movetostart;
      }
    }
    if (uCount>=HISTORYLIMIT) {
      uCount--;
      freesafe(pHist->pszBuf[uCount],"HistoryAddString");
      goto copytoend;
    }
    pHist->pszBuf=(char **)reallocsafeNULL(pHist->pszBuf,(uCount+1)*sizeof(*pHist->pszBuf),"HistoryAddString");
    pHist->puBufLen=(unsigned *)reallocsafeNULL(pHist->puBufLen,(uCount+1)*sizeof(*pHist->puBufLen),"HistoryAddString");
    pHist->fFlags=(WORD *)reallocsafeNULL(pHist->fFlags,(uCount+1)*sizeof(*pHist->fFlags),"HistoryAddString");
    if (pHist->pszBuf && pHist->puBufLen && pHist->fFlags) {
      pHist->uCount++;
copytoend:
      pHist->pszBuf[uCount]=memdupsafe(sBuf,uBufLen,"HistoryAddString");
      pHist->puBufLen[uCount]=uBufLen;
      pHist->fFlags[uCount]=fFlags;
movetostart: // Move 0-based:uCount to the 0th position
      if (uCount>=1 && pHist->uCount>1) {
        unsigned uTempFlag=pHist->fFlags[uCount];
        unsigned uTempLen=pHist->puBufLen[uCount];
        char *szTemp=pHist->pszBuf[uCount];
        memmove(pHist->pszBuf+1,pHist->pszBuf,uCount*sizeof(*pHist->pszBuf));
        memmove(pHist->puBufLen+1,pHist->puBufLen,uCount*sizeof(*pHist->puBufLen));
        memmove(pHist->fFlags+1,pHist->fFlags,uCount*sizeof(*pHist->fFlags));
        pHist->fFlags[0]=uTempFlag;
        pHist->puBufLen[0]=uTempLen;
        pHist->pszBuf[0]=szTemp;
      }
    } else HistoryInit(pHist,TRUE);
    HistoryLoadStrings(pHist,hwndCB,szTitle); // bad performance but way easier than the alternative
  }
}

// returns TRUE if successful
static BOOL ReplaceCheck(HWND hDlg,FINDREPLACEX *pfr) {
      if (!pfr->lpstrReplaceWith || (pfr->InternalFlags&FRI_DIRTYREPLACE)) {
        GetWindowTextUTF8arm(pfr->hwndEditor,GetDlgItem(hDlg,IDC_EDTREPLACE),&pfr->lpstrReplaceWith,&pfr->wReplaceWithSz,&pfr->wReplaceWithLen); if (!pfr->lpstrReplaceWith) return FALSE;
        pfr->InternalFlags&=~FRI_DIRTYREPLACE; //MessageBox(hDlg,"Replace updated","???",MB_ICONSTOP);
        HistoryAddString(&g_ReplHist,GetDlgItem(hDlg,g_fISNT?IDC_CMBREPLHISTW:IDC_CMBREPLHISTA),pfr->lpstrReplaceWith,pfr->wReplaceWithLen+1,pfr->InternalFlags&FHFLAG_UNICODE,"Replace");
      }
      return TRUE;
}

static void EnableFindButtons(FINDREPLACEX *pfr,HWND hDlg,BOOL twl) {
  HWND hwndFocus=GetFocus();
  EnableWindow(GetDlgItem(hDlg,IDC_PSHFIND),twl);
  EnableWindow(GetDlgItem(hDlg,IDC_PSHCOUNT),twl);
  EnableWindow(GetDlgItem(hDlg,IDC_PSHREPLACEFIND),twl && !(pfr->InternalFlags&FRI_NOTFOUND));
  EnableWindow(GetDlgItem(hDlg,IDC_EDTREPLACECT),twl && !(pfr->InternalFlags&FRI_NOTFOUND));
  EnableWindow(GetDlgItem(hDlg,IDC_UPDOWN),twl && !(pfr->InternalFlags&FRI_NOTFOUND));
  EnableWindow(GetDlgItem(hDlg,IDC_PSHREPLACEREST),twl && !(pfr->InternalFlags&FRI_NOTFOUND));
  if (!IsWindowEnabled(hwndFocus)) SetFocus(GetNextDlgTabItem(hDlg,hwndFocus,(GetKeyState (VK_SHIFT)&0x8000)?TRUE:FALSE));
}

static BOOL FindCheck(HWND hDlg,FINDREPLACEX *pfr) {
      if (!pfr->lpstrFindWhat || (pfr->InternalFlags&FRI_DIRTYFIND)) {
        GetWindowTextUTF8arm(pfr->hwndEditor,GetDlgItem(hDlg,IDC_EDTFIND),&pfr->lpstrFindWhat,&pfr->wFindWhatSz,&pfr->wFindWhatLen); if (!pfr->lpstrFindWhat) return FALSE;
        pfr->InternalFlags&=~FRI_DIRTYFIND; //MessageBox(hDlg,"Find updated","???",MB_ICONSTOP);
        HistoryAddString(&g_FindHist,GetDlgItem(hDlg,g_fISNT?IDC_CMBFINDHISTW:IDC_CMBFINDHISTA),pfr->lpstrFindWhat,pfr->wFindWhatLen+1,pfr->InternalFlags&FHFLAG_UNICODE,"Find");
        pfr->InternalFlags|=FRI_NOTFOUND;
        EnableFindButtons(pfr,hDlg,pfr->wFindWhatLen?TRUE:FALSE);
      }
      return TRUE;
}

// hDlg is available as pfr->hwndSelf but passing it separately allows code to be moved out of the DialogProc without changes
static void ApplyChecks(HWND hDlg,FINDREPLACEX *pfr) {
//#define LOADABLECHECKSSCINTILLA (IDC_CHXMATCHCASE|IDC_CHXWHOLEWORD|IDC_CHXSTARTWORD|IDC_CHXREGEX)
//#define LOADABLECHECKSPERSISTENT (IDC_CHXRECURSIVE|IDC_CHXLINE1ST|IDC_CHXVISIBLE|IDC_CHXREPLCASE)
#define PROCESSCHECKSA \
      PROCESSCHECK(IDC_CHXMATCHCASE,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_MATCHCASE,FALSE,"MatchCase",9);\
      PROCESSCHECK(IDC_CHXWHOLEWORD,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_WHOLEWORD,FALSE,"WholeWord",9);\
      PROCESSCHECK(IDC_CHXSTARTWORD,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_WORDSTART,FALSE,"WordStart",9);\
      PROCESSCHECK(IDC_CHXREGEX,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_REGEXP,TRUE,"RegEx",5);\
      PROCESSCHECK(IDC_CHXRECURSIVE,pfr->PersistentFlags,pfr->StorageFlags,FRP_RECURSIVE,FALSE,"RecursiveReplace",16);\
      PROCESSCHECK(IDC_CHXLINE1ST,pfr->PersistentFlags,pfr->StorageFlags,FRP_LINE1ST,TRUE,"FirstOnLine",11);\
      PROCESSCHECK(IDC_CHXVISIBLE,pfr->PersistentFlags,pfr->StorageFlags,FRP_VISIBLE,TRUE,"VisibleTextOnly",15);\
      PROCESSCHECK(IDC_CHXREPLCASE,pfr->PersistentFlags,pfr->StorageFlags,FRP_REPLCASE,FALSE,"ReplaceMatchCase",16);
#define PROCESSCHECKSB \
      PROCESSCHECK(IDC_CHXWRAP,pfr->PersistentFlags,pfr->StorageFlags,FRP_WRAP,FALSE,"Wrap",4);\
      PROCESSCHECK(IDC_CHXINCREMENTAL,pfr->PersistentFlags,pfr->StorageFlags,FRP_INCREMENTAL,TRUE,"Incremental",11);\
      PROCESSCHECK(IDC_CHXSELECTED,pfr->PersistentFlags,pfr->StorageFlags,FRP_SELECTED,TRUE,"SelectedText",12);\
      PROCESSCHECK(IDC_CHXAUTOGRAB,pfr->PersistentFlags,pfr->StorageFlags,FRP_AUTOGRABFIND,FALSE,"AutoGrab",8);
#define PROCESSCHECK1(cs,flg,sflg,bt,ck,nm,ln) SendDlgItemMessage(hDlg,cs,BM_SETCHECK,((flg)&(bt))?BST_CHECKED:BST_UNCHECKED,0)
#define PROCESSCHECK PROCESSCHECK1
      CheckFlags(hDlg,pfr);
      PROCESSCHECKSA
      PROCESSCHECKSB /* Used here to set the initial checks */
#undef PROCESSCHECK
}

// szFolder is a known folder and we need to ensure that it ends in a slash if there's enough room
// the new length is returned
static unsigned FolderPadSlash(char *szFolder,unsigned uFolderSz,unsigned uFolderLen) {
  if (uFolderLen==(unsigned)-1) uFolderLen=strlen(szFolder);
  if (uFolderSz && uFolderLen<uFolderSz-1) {
      if (!uFolderLen || szFolder[uFolderLen-1]!='\\') szFolder[uFolderLen++]='\\';
      szFolder[uFolderLen]='\0';
  }
  return uFolderLen;
}

// duplicates the folder portion of szPathSrc into szPathDst, overwriting an existing path if necessary
// the new length of szPathDst is returned, or (unsigned)-1 if failed. Notice -1 forces the next call to calculate strlen() so it's only extra work, not an invalid length
static unsigned PathDup(char *szPathDst,unsigned uPathDstSz,unsigned uPathDstLen,const char *szPathSrc,unsigned uPathSrcLen) {
  if (uPathSrcLen==(unsigned)-1) uPathSrcLen=strlen(szPathSrc);
  if (uPathDstLen==(unsigned)-1) uPathDstLen=strlen(szPathDst);
  if (uPathSrcLen && uPathDstSz) {
    unsigned uFolderSrcLen;
    uFolderSrcLen=(const char *)memchrX(szPathSrc+uPathSrcLen-1,szPathSrc-1,'\\')-szPathSrc+1;
    if (uFolderSrcLen) {
      unsigned uFolderDstLen;
      int iDif;
      uFolderDstLen=(const char *)memchrX(szPathDst+uPathDstLen-1,szPathDst-1,'\\')-szPathDst+1;
      iDif=uFolderSrcLen-uFolderDstLen;
      if (uPathDstLen + iDif <= uPathDstSz-1) {
        if (iDif) memmove(szPathDst+uFolderDstLen+iDif,szPathDst+uFolderDstLen,uPathDstLen-uFolderDstLen+1);
        memcpy(szPathDst,szPathSrc,uFolderSrcLen);
        return uPathDstLen + iDif;
      }
    }
  }
  return (unsigned)-1;
}

#if 0
// Removes the folder portion of szPathDst
// the new length of szPathDst is returned
static unsigned PathDel(char *szPathDst,unsigned uPathDstLen) {
  if (uPathDstLen==(unsigned)-1) uPathDstLen=strlen(szPathDst);
  if (uPathDstLen) {
    unsigned uFolderDstLen=memchrX(szPathDst+uPathDstLen-1,szPathDst-1,'\\')-szPathDst+1;
    if (uFolderDstLen) {
      memmove(szPathDst,szPathDst+uFolderDstLen,uPathDstLen-uFolderDstLen+1);
      uPathDstLen -= uFolderDstLen;
    }
  }
  return uPathDstLen;
}
#endif

//char g_szTitle[MAX_PATH],g_szFile[MAX_PATH],g_szInit[MAX_PATH];
static void DirManage(HWND hwndFolders,WORD wFunc) {
  unsigned uCursel=SENDMSGTOED(hwndFolders,CB_GETCURSEL,0,0);
  unsigned uTextSz=SENDMSGTOED(hwndFolders,CB_GETLBTEXTLEN,uCursel,0)+1;  // 1 byte for \0
  char *szBuf=(char *)mallocsafe(uTextSz,"DirManage"); if (!szBuf) return;
  char *szEnd = szBuf+SENDMSGTOED(hwndFolders,CB_GETLBTEXT,uCursel,szBuf);
  char *szFolder=szBuf;
  szFolder=memchrX(szFolder,szEnd,':'); if (szFolder>=szEnd) szFolder=szBuf; else szFolder++;
  while(szFolder<szEnd && isspace(*szFolder)) szFolder++;
  g_szFile[0]='\0'; //MessageBox(0,szFolder,g_szLastLoadedFile,MB_OK);
  g_szInit[0]='\0';
  if (wFunc==IDC_PSHLOAD) {
    if (*szFolder) {
      strncpymem(g_szInit,NELEM(g_szInit),szFolder,szEnd-szFolder);
      FolderPadSlash(g_szInit,NELEM(g_szInit),(unsigned)-1);
    } else PathDup(g_szInit,NELEM(g_szInit),(unsigned)-1,g_szLastLoadedFile,(unsigned)-1);
  } else {
    strncpymem(g_szFile,NELEM(g_szFile),g_szLastLoadedFile,(unsigned)-1);
  }
  freesafe(szBuf,"DirManage");
}

static struct _ECHILD {
  HWND *hwndChildren;
  unsigned cChildren;
} g_ec;

static BOOL CALLBACK EnumChildProc(HWND hwndChild,LPARAM lParam) {
  struct _ECHILD *pec=(struct _ECHILD *)lParam;
  pec->hwndChildren=(HWND *)reallocsafeNULL(pec->hwndChildren,sizeof(*pec->hwndChildren)*(pec->cChildren+1),"EnumChildProc");
  if (pec->hwndChildren) {
    pec->hwndChildren[pec->cChildren]=hwndChild;
    pec->cChildren++;
  } else pec->cChildren=0;
  return TRUE;
}

// GetWindowRect() of a child window relative to its parent appropriate for a MoveWindow
// If this is to be called many times, calculate hwndParent and pass it in each time, otherwise hwndParent=NULL for automatic calculation
// if the window has changed from last time, call hwndParent==NULL to clear the point cache
static BOOL GetChildRect(HWND hwndParent,HWND hwndChild,RECT *prcChild) {
  static HWND hwndParentPoint=NULL;
  static POINT ptParent={0,0}; // this is reused as long as the same hwndParent comes in, potentially bad for threading

  if (hwndParent || (!(hwndParentPoint=NULL) && (hwndParent=GetParent(hwndChild)))) {
    if (hwndParentPoint==hwndParent || (!(ptParent.x=ptParent.y=0) && ClientToScreen(hwndParent,&ptParent))) {
      hwndParentPoint=hwndParent;
      if (GetWindowRect(hwndChild,prcChild)) { //MessageBoxFree(0,smprintf(TEXT("Parent:[0,0]->[%d,%d]\r\nChild(left:%u,top:%u,right:%u,bottom:%u,width:%u,height:%u)"),ptParent.x,ptParent.y,prcChild->left,prcChild->top,prcChild->right,prcChild->bottom,prcChild->right-prcChild->left,prcChild->top,prcChild->bottom,prcChild->bottom-prcChild->top),TEXT("???"),MB_OK);
        prcChild->left  -= ptParent.x;
        prcChild->right -= ptParent.x;
        prcChild->top   -= ptParent.y;
        prcChild->bottom-= ptParent.y;
        return(TRUE);
      }
    }
  }
  memset(prcChild,0,sizeof(*prcChild)); // fail massively
  return(FALSE);
}

#if 0
static BOOL MoveWindowNoFlicker(HWND hwnd,int X,int Y,int nWidth,int nHeight,RECT *prcExisting) {
  return (X != prcExisting->left || Y != prcExisting->top || nWidth != prcExisting->right-prcExisting->left || nHeight != prcExisting->bottom-prcExisting->top)?MoveWindow(hwnd,X,Y,nWidth,nHeight,TRUE):TRUE;
}
#endif

static void SetWindowCharacter(HWND hwnd,char c) {
  char szText[3];
  switch(c) {
  case 0: szText[0]='\0'; break;
  case '\n':
  case '\r':
    strcpy(szText,"\\n");
    break;
  default:
    szText[0]=c;
    szText[1]='\0';
    break;
  }
  SetWindowText(hwnd,szText);
  SENDMSGTOED(hwnd,EM_LIMITTEXT,2,0); // allows for \n
}

static char GetWindowCharacter(HWND hwnd) {
  char szText[3];
  unsigned uTextLen=GetWindowText(hwnd,szText,sizeof(szText));
  if (uTextLen==1) return szText[0];
  else if (uTextLen==2 && szText[0]=='\\') switch(szText[1]) {
    case 'n':
    case 'r':
      return '\n';
  } //MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("uTextLen:%u\r\n%s"),uTextLen,szText),TEXT("???"),MB_OK);
  return '\0';
}

static char ValidateWindowCharacter(HWND hwnd) {
  char c=GetWindowCharacter(hwnd);
  SetWindowCharacter(hwnd,c);
  return c;
}

// http://www.xploiter.com/programming/c/borland/869.html
// CNTRDLG.C Copyright (C) 1992 Borland International, Inc. All Rights Reserved Source for the UICenterDialog function
static void UICenterDialog(LPFINDREPLACEX pfr) {
  int cxScreen = GetSystemMetrics(SM_CXSCREEN);
  int cyScreen = GetSystemMetrics(SM_CYSCREEN);
  if (cxScreen != pfr->cxScreen || cyScreen != pfr->cyScreen) {
    RECT      rc;
    GetWindowRect(pfr->hwndSelf, &rc); // the dialog
    int cxDlg = rc.right - rc.left;
    int cyDlg = rc.bottom - rc.top;
    GetWindowRect(pfr->hwndOwner, &rc);

    int xDlg = ((rc.right - rc.left) - cxDlg) / 2 + rc.left; // Determine new dialog X and Y coordinate
    int yDlg = ((rc.bottom - rc.top) - cyDlg) / 2 + rc.top; //{ TCHAR debug[256]; _sntprintf(debug,sizeof(debug),TEXT("xDlg:%d yDlg:%d cxDlg:%d cyDlg:%d"),xDlg,yDlg,cxDlg,cyDlg); MessageBox(0,debug,TEXT("???"),MB_OK); }

    if (xDlg < 0 || yDlg < 0 || xDlg+cxDlg > cxScreen || yDlg+cyDlg > cyScreen) { // Center the dialog using new coordinates
      xDlg = (cxScreen - cxDlg) / 2; // If by centering the dialog on top of its parent it would not be 100% visible, then center it on the Windows desktop instead.
      yDlg = (cyScreen - cyDlg) / 2;
    }
    if (cxDlg>cxScreen) cxDlg=cxScreen;//friendly to 640x480 screens
    if (cyDlg>cyScreen) cyDlg=cyScreen;
    SetRect(&pfr->rcDlg,xDlg,yDlg,xDlg+cxDlg,yDlg+cyDlg);
  }
  MoveWindow(pfr->hwndSelf,pfr->rcDlg.left,pfr->rcDlg.top,pfr->rcDlg.right-pfr->rcDlg.left,pfr->rcDlg.bottom-pfr->rcDlg.top, TRUE); // This usually occurs while the window is hidden anyways
} /*DISCLAIMER: You have the right to use this technical information subject to the terms of the No-Nonsense License Statement that you received with the Borland product to which this information pertains.*/

// this DlgProc is substantially less buggy than Windows own for FindText/ReplaceText
// In here we try to minimize editor specific stuff and only perform things that apply generally to F&R dialogs.
// Editor specific stuff goes into cbFindReplaceX. This separation will help both routines to be more readable.
static INT_PTR CALLBACK FindReplaceDlgProc(HWND hDlg, UINT message,WPARAM wParam, LPARAM lParam) {
  static FINDREPLACEX *pfr=NULL; // not thread safe but fairly easy to fix with a storage stack
  static unsigned cyClientHeight,cyOrigWindowHeight;

  switch (message) {
  case WM_INITDIALOG :
    pfr = (FINDREPLACEX *)lParam;
    if (!IsBadWritePtr(pfr,sizeof(*pfr)) && (pfr->hwndSelf=hDlg) && !pfr->cbFindReplaceX(FRC_DIALOGCREATE,pfr)) {
      ApplyChecks(hDlg,pfr);
      CheckRadioButton(hDlg,IDC_RADUP,IDC_RADDOWN,(pfr->PersistentFlags&FRP_UP)?IDC_RADUP:IDC_RADDOWN);
      SendDlgItemMessage(hDlg,IDC_UPDOWN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hDlg,IDC_EDTREPLACECT),0);
      SendDlgItemMessage(hDlg,IDC_UPDOWN,UDM_SETRANGE32,1,2048576);
      {
        HWND hDropDown=GetDlgItem(hDlg,IDC_CMBREGEX);
        unsigned i; for(i=0; i<NELEM(g_tszRegex); i++) SENDMSGTOED(hDropDown,CB_ADDSTRING,0,g_tszRegex[i]);
        SENDMSGTOED(hDropDown,CB_SETCURSEL,0,0);
      }
      ShowWindow(GetDlgItem(hDlg,IDC_CMBFINDHISTA),g_fISNT?SW_HIDE:SW_SHOW); //ComboBoxEx32 refuses to display on Win9x
      ShowWindow(GetDlgItem(hDlg,IDC_CMBFINDHISTW),g_fISNT?SW_SHOW:SW_HIDE); //so Win9x users don't get to see their UNICODE
      ShowWindow(GetDlgItem(hDlg,IDC_CMBREPLHISTA),g_fISNT?SW_HIDE:SW_SHOW);
      ShowWindow(GetDlgItem(hDlg,IDC_CMBREPLHISTW),g_fISNT?SW_SHOW:SW_HIDE);
      HistoryLoadStrings(&g_FindHist,GetDlgItem(hDlg,g_fISNT?IDC_CMBFINDHISTW:IDC_CMBFINDHISTA),"Find");
      HistoryLoadStrings(&g_ReplHist,GetDlgItem(hDlg,g_fISNT?IDC_CMBREPLHISTW:IDC_CMBREPLHISTA),"Replace");
      RepositoryLoadStrings(pfr,GetDlgItem(hDlg,IDC_CMBFOLDERS));
      EnableFindButtons(pfr,hDlg,pfr->wFindWhatLen?TRUE:FALSE);
      EnableReplaceButtons(hDlg,pfr->wReplaceWithLen?TRUE:FALSE);
      SetWindowCharacter(GetDlgItem(hDlg,IDC_EDTSEQUENCE),pfr->cSequence);
      SetWindowCharacter(GetDlgItem(hDlg,IDC_EDTCOUNTER),pfr->cCounter);
      {
        memset(&g_ec,0,sizeof(g_ec)); EnumChildWindows(hDlg,EnumChildProc,(LPARAM)&g_ec); //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("cChildren:%u"),g_ec.cChildren),TEXT("???"),MB_OK);
        RECT rcDlg;
        GetClientRect(hDlg,&rcDlg); cyClientHeight=rcDlg.bottom;
        GetWindowRect(hDlg,&rcDlg); cyOrigWindowHeight=rcDlg.bottom-rcDlg.top-100; // The dialog was drawn for multiline. Now that the dialog is resizable, it is permissable to smallinize it to the minimum size of the edit boxes
        unsigned temp=pfr->PersistentFlags; // UICenterDialog() generates a WM_SIZE which clears FRP_MAXIMIZED
        UICenterDialog(pfr);
        if (temp&FRP_MAXIMIZED) ShowWindow(hDlg,SW_MAXIMIZE);
      }
      ShowWindow(hDlg,SW_SHOW);
      return TRUE;
    }
    DestroyWindow(hDlg); // fall through: Destroying the dialog before it starts supresses the call to WM_DESTROY
  case WM_DESTROY:
    if (!IsBadWritePtr(pfr,sizeof(*pfr))) {
      pfr->cbFindReplaceX(FRC_DIALOGTERM,pfr); // we don't care what they return here!
      pfr->hwndSelf=(HWND)SENDMSGTOED(pfr->hwndOwner,NPPM_MODELESSDIALOG,MODELESSDIALOGREMOVE,hDlg);
      SetForegroundWindow(pfr->hwndOwner);
      pfr=NULL;
    }
    if (g_ec.hwndChildren) freesafe(g_ec.hwndChildren,"FindReplaceDlgProc:WM_DESTROY");
    break;
  case WM_NOTIFY:
    switch (wParam) {
    case IDC_EDTFIND: {
        struct SCNotification *scn=(struct SCNotification *)lParam;
        if (scn->nmhdr.code==SCN_MODIFIED && (scn->modificationType&(SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT)) ) {
          EnableFindButtons(pfr,hDlg,SendDlgItemMessage(hDlg,IDC_EDTFIND, SCI_GETLENGTH, 0, 0)?TRUE:FALSE);
          pfr->InternalFlags|=FRI_DIRTYFIND; //MessageBox(0,"Notification","???",MB_OK);
          if (!(pfr->InternalFlags&FRI_BLOCKUPDATE)) SetCurselOptional(GetDlgItem(hDlg,g_fISNT?IDC_CMBFINDHISTW:IDC_CMBFINDHISTA),0);
          if (pfr->PersistentFlags&FRP_INCREMENTAL) {
            SENDMSGTOED(pfr->hwndEditor,SCI_GOTOPOS,pfr->ep1,0);
            PostMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_PSHFIND,IDC_CHXINCREMENTAL),lParam);
          }
          return TRUE;
        }
      } break ;
    case IDC_EDTREPLACE: {
      struct SCNotification *scn=(struct SCNotification *)lParam;
      if (scn->nmhdr.code==SCN_MODIFIED && (scn->modificationType&(SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT)) ) {
        EnableReplaceButtons(hDlg,SendDlgItemMessage(hDlg,IDC_EDTREPLACE, SCI_GETLENGTH, 0, 0)?TRUE:FALSE);
        pfr->InternalFlags|=FRI_DIRTYREPLACE; //MessageBox(0,"Notification","???",MB_OK);
        if (!(pfr->InternalFlags&FRI_BLOCKUPDATE)) SetCurselOptional(GetDlgItem(hDlg,g_fISNT?IDC_CMBREPLHISTW:IDC_CMBREPLHISTA),0);
        return TRUE;
      }
    }
    } break;
  case WM_GETMINMAXINFO:
    ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=cyOrigWindowHeight;
    return TRUE;
  case WM_SIZE: if (pfr) {
#define cyClientHeightNew ((unsigned)(USHORT)HIWORD(lParam))
#define cyClientWidthNew ((unsigned)(USHORT)LOWORD(lParam))
      RECT rcEditFind,rcEditReplace;
      GetChildRect(NULL,GetDlgItem(hDlg,IDC_EDTREPLACE),&rcEditReplace);
      GetChildRect(hDlg,GetDlgItem(hDlg,IDC_EDTFIND),&rcEditFind);
      unsigned cyEdits=rcEditReplace.bottom-rcEditFind.top;
      if (cyClientHeight != cyClientHeightNew) {
        if (cyClientHeight>cyClientHeightNew) { // prevent double flashes for edit windows that overlap InvalidateRect() when resizing smaller
          MoveWindow(GetDlgItem(hDlg,IDC_EDTREPLACE),0,rcEditFind.top,0,0,FALSE);
          MoveWindow(GetDlgItem(hDlg,IDC_EDTFIND),0,rcEditFind.top,0,0,FALSE);
        }
        RECT rcChild;
        LONG yMin=SIGNEDMAXPOSITIVE;
        unsigned i; for(i=g_ec.cChildren; i; ) { // find and move all dialog items below the edit-replace window
          i--;
          GetChildRect(hDlg,g_ec.hwndChildren[i],&rcChild);
          if (rcChild.top>=rcEditReplace.bottom) { //MessageBoxFree(0,smprintf(TEXT("Child[%u](left:%u,top:%u,right:%u,bottom:%u,width:%u,height:%u,dy:%i,yMin:%i)"),i,rcChild.left,rcChild.top,rcChild.right,rcChild.bottom,rcChild.right-rcChild.left,rcChild.bottom-rcChild.top,cyClientHeightNew-cyClientHeight,yMin),TEXT("???"),MB_OK);
            rcChild.top+=cyClientHeightNew-cyClientHeight;
            rcChild.bottom+=cyClientHeightNew-cyClientHeight;
            MoveWindow(g_ec.hwndChildren[i],rcChild.left,rcChild.top,rcChild.right-rcChild.left,rcChild.bottom-rcChild.top,FALSE);
            if (yMin>rcChild.top) yMin=rcChild.top;
          }
        }
        GetClientRect(hDlg,&rcChild);
        rcChild.top=yMin-1;
        InvalidateRect(hDlg,&rcChild,TRUE);
        cyClientHeight = cyClientHeightNew;
        cyEdits=yMin-rcEditFind.top;
      } //SetWindowTextFree(GetDlgItem(pfr->hwndSelf,stc1),smprintf(TEXT("cyEdits:%i"),cyEdits));
      cyEdits /= 2; // resize edit-find and edit-replace to maximum width and each get half of available vertical space
      MoveWindow(GetDlgItem(hDlg,IDC_EDTREPLACE),0,rcEditFind.top+cyEdits,cyClientWidthNew,cyEdits,TRUE);
      MoveWindow(GetDlgItem(hDlg,IDC_EDTFIND),0,rcEditFind.top,cyClientWidthNew,cyEdits,TRUE);
      GetChildRect(hDlg,GetDlgItem(hDlg,stc1),&rcEditReplace);
      MoveWindow(GetDlgItem(hDlg,stc1),0,rcEditReplace.top,cyClientWidthNew,rcEditReplace.bottom-rcEditReplace.top,TRUE);
      if (wParam == SIZE_RESTORED) {
        GetWindowRect(hDlg,&pfr->rcDlg);
        pfr->PersistentFlags&=~FRP_MAXIMIZED;
      } else pfr->PersistentFlags|=FRP_MAXIMIZED;
    }
#undef cyClientHeightNew
#undef cyClientWidthNew
    return TRUE;
  case WM_MOVE: // the lParam client coordinates are not desired here.
    GetWindowRect(hDlg,&pfr->rcDlg);
    return TRUE;
  case WM_COMMAND:
    if (pfr) switch (LOWORD (wParam)) { // EN_CHANGE seems to fire before pfr is set for the first time, probably while the dialog is built
    case IDC_EDTCOLUMNS:
      if (HIWORD(wParam)==EN_CHANGE) {
        pfr->InternalFlags|=FRI_DIRTYCOLUMN;
        return TRUE;
      }
      break ;
    case IDC_EDTSEQUENCE:
      if (HIWORD(wParam)==EN_CHANGE) {
        pfr->cSequence=GetWindowCharacter((HWND)lParam);
        return TRUE;
      }
      break ;
    case IDC_EDTCOUNTER:
      if (HIWORD(wParam)==EN_CHANGE) {
        pfr->cCounter=GetWindowCharacter((HWND)lParam);
        SetWindowTextFree(GetDlgItem(hDlg,stc1),smprintf(pfr->cCounter?"Counter usage: %c(id,start,increment,width)":"",pfr->cCounter));
        return TRUE;
      }
      break ;
    case IDC_PSHLOAD:
#if NPPDEBUG
      g_ofn.hwndOwner         = hDlg ;
      g_ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
      //g_ofn.lpstrFile[0]='\0';
      DirManage(GetDlgItem(hDlg,IDC_CMBFOLDERS),LOWORD(wParam));
      if (GetOpenFileName(&g_ofn)) {
#if NPPDEBUG
        //XMLTest(g_ofn.lpstrFile,"C:\\TEST.XML");
#endif
        struct MICROXML mx;
        if (XMLReadOpen(&mx,g_ofn.lpstrFile,O_RDONLY,O_DENYWRITE)) {
          pfr->PersistentFlags &= ~(FRP_INCREMENTAL/*|LOADABLECHECKSPERSISTENT*/); // This might not be desirable in case people want to have certain settings persist across loaded sets
          /*pfr->StorageFlags &= ~(FRP_INCREMENTAL|LOADABLECHECKSPERSISTENT);
          pfr->ScintillaFlags &= ~(LOADABLECHECKSSCINTILLA); pfr->SciStorageFlags &= ~(LOADABLECHECKSSCINTILLA);*/
          int iXMLRead=0;
          while((iXMLRead=XMLReadLevel(&mx,0,iXMLRead))>MX_DOWNLIMIT) switch(iXMLRead) {
          case MX_LEVELUP:
            mx.iUsed=TRUE;
            if (mx.uValueLen == 14 && !memicmp(mx.sValue,"TextFX:Replace",14)) while((iXMLRead=XMLReadLevel(&mx,1,iXMLRead))>MX_DOWNLIMIT) switch(iXMLRead) {
#define PROCESSCHECK4(cs,flg,sflg,bt,ck,nm,ln) if (!memicmp(mx.sAttribute,"option:"nm,7+(ln))) do {(sflg)&=~(bt); if (mx.uValueConvertedLen && mx.sValueConverted[0]=='1') (flg)|=(bt); else (flg)&=~(bt); mx.iUsed=TRUE;} while(0)
#define PROCESSCHECK PROCESSCHECK4
            case MX_ATTRIBUTE:
              switch(mx.uAttributeLen) {
              case 12:
                PROCESSCHECK(IDC_CHXREGEX,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_REGEXP,TRUE,"RegEx",5);
                break;
              case 16:
                PROCESSCHECK(IDC_CHXMATCHCASE,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_MATCHCASE,FALSE,"MatchCase",9);
                else PROCESSCHECK(IDC_CHXWHOLEWORD,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_WHOLEWORD,FALSE,"WholeWord",9);
                else PROCESSCHECK(IDC_CHXSTARTWORD,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_WORDSTART,FALSE,"WordStart",9);
                break;
              case 18:
                PROCESSCHECK(IDC_CHXLINE1ST,pfr->PersistentFlags,pfr->StorageFlags,FRP_LINE1ST,TRUE,"FirstOnLine",11);
                break;
              case 22:
                PROCESSCHECK(IDC_CHXVISIBLE,pfr->PersistentFlags,pfr->StorageFlags,FRP_VISIBLE,TRUE,"VisibleTextOnly",15);
                break;
              case 23:
                PROCESSCHECK(IDC_CHXRECURSIVE,pfr->PersistentFlags,pfr->StorageFlags,FRP_RECURSIVE,FALSE,"RecursiveReplace",16);
                else PROCESSCHECK(IDC_CHXREPLCASE,pfr->PersistentFlags,pfr->StorageFlags,FRP_REPLCASE,FALSE,"ReplaceMatchCase",16);
                else if (!memicmp(mx.sAttribute,"option:CounterCharacter",23)) {if (mx.uValueConvertedLen) SetWindowCharacter(GetDlgItem(hDlg,IDC_EDTCOUNTER),pfr->cCounter=mx.sValueConverted[0]); mx.iUsed=TRUE; }
                break;
              case 24:
                if (!memicmp(mx.sAttribute,"option:SequenceCharacter",24)) {if (mx.uValueConvertedLen) SetWindowCharacter(GetDlgItem(hDlg,IDC_EDTSEQUENCE),pfr->cSequence=mx.sValueConverted[0]); mx.iUsed=TRUE; }
              }
              break;
#undef PROCESSCHECK
            case MX_LEVELUP:
              mx.iUsed=TRUE;
              switch(mx.uValueLen) {
              case 8: if (!memicmp(mx.sValue,"FindText",8)) while((iXMLRead=XMLReadLevel(&mx,2,iXMLRead))>MX_DOWNLIMIT) switch(iXMLRead) {
                case MX_TEXT: {
                    char *sStart,*sEnd;
                    sStart=memchrX(mx.sValueConverted,mx.sValueConverted+mx.uValueConvertedLen,'/');
                    sEnd=memchrX(mx.sValueConverted+mx.uValueConvertedLen-1,sStart-1,'/');
                    sStart++;
                    SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTFIND),sStart,(sEnd>=sStart)?sEnd-sStart:0,-1);
                    pfr->InternalFlags|=(FRI_DIRTYFIND);
                    mx.iUsed=TRUE;
                  } break;
                }
                break;
              case 10: if (!memicmp(mx.sValue,"EditorText",10)) while((iXMLRead=XMLReadLevel(&mx,2,iXMLRead))>MX_DOWNLIMIT) switch(iXMLRead) {
                case MX_TEXT: {
                    char *sStart,*sEnd;
                    sStart=memchrX(mx.sValueConverted,mx.sValueConverted+mx.uValueConvertedLen,'/');
                    sEnd=memchrX(mx.sValueConverted+mx.uValueConvertedLen-1,sStart-1,'/');
                    sStart++;
                    SENDMSGTOED(pfr->hwndEditor,SCI_BEGINUNDOACTION,0,0);
                    SENDMSGTOED(pfr->hwndEditor,SCI_SETTEXT,0,"(This text has been loaded by a Find & Replace\nPress Undo to revert to your original text\n\n");
                    SENDMSGTOED(pfr->hwndEditor,SCI_DOCUMENTEND,0,0);
                    SENDMSGTOED(pfr->hwndEditor,SCI_ADDTEXT,(sEnd>=sStart)?sEnd-sStart:0,sStart);
                    SENDMSGTOED(pfr->hwndEditor,SCI_ENDUNDOACTION,0,0);
                    mx.iUsed=TRUE;
                  } break;
                }
              case 11: if (!memicmp(mx.sValue,"ReplaceText",11)) while((iXMLRead=XMLReadLevel(&mx,2,iXMLRead))>MX_DOWNLIMIT) switch(iXMLRead) {
                case MX_TEXT: {
                    char *sStart,*sEnd;
                    sStart=memchrX(mx.sValueConverted,mx.sValueConverted+mx.uValueConvertedLen,'/');
                    sEnd=memchrX(mx.sValueConverted+mx.uValueConvertedLen-1,sStart-1,'/');
                    sStart++;
                    SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTREPLACE),sStart,(sEnd>=sStart)?sEnd-sStart:0,-1);
                    pfr->InternalFlags|=(FRI_DIRTYREPLACE);
                    mx.iUsed=TRUE;
                  } break;
                }
                break;
              }
              break;
            }
            break;
          case MX_COMMENT: if (mx.uValueLen>4) { // --?--
              char *sStart,*sEnd;
              sEnd  =mx.sValue+mx.uValueLen;
              sStart=memstr(mx.sValue,sEnd,"--",2)+2;
              sEnd  =memstr(sEnd-1,mx.sValue-1,"--",2);
              if (sStart<sEnd) {
                *sEnd='\0';
                SetWindowText(GetDlgItem(hDlg,stc1),sStart);
              }
            }
            mx.iUsed=TRUE;
            break;
          }
          if (iXMLRead!=MX_DONE) MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("Failure: [%u]%s"),mx.sValue,mx.uValueLen),TEXT("Find & Replace"),MB_OK);
          XMLReadClose(&mx);
          ApplyChecks(hDlg,pfr);
          strncpymem(g_szLastLoadedFile,NELEM(g_szLastLoadedFile),g_ofn.lpstrFile,(unsigned)-1);
        }
      }
      //MessageBox(0,g_ofn.lpstrInitialDir,"???",MB_OK);
      break;
#endif
    case IDC_PSHSAVE:
#if NPPDEBUG
      g_ofn.hwndOwner         = hDlg;
      g_ofn.Flags             = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
      DirManage(GetDlgItem(hDlg,IDC_CMBFOLDERS),LOWORD(wParam));
      if (ReplaceCheck(hDlg,pfr) && FindCheck(hDlg,pfr) && GetSaveFileName(&g_ofn)) {
        char *sComment=NULL;
        unsigned uComment=(unsigned)-1;
        struct MICROXML mx;
        if (XMLReadOpen(&mx,g_ofn.lpstrFile,O_RDONLY,O_DENYWRITE)) { // preserve existing comment
          if (MX_COMMENT==XMLReadLevel(&mx,0,0)) {
              char *sStart,*sEnd;
              sEnd  =mx.sValue+mx.uValueLen;
              sStart=memstr(mx.sValue,sEnd,"--",2)+2;
              sEnd  =memstr(sEnd-1,mx.sValue-1,"--",2);
              if (sStart<sEnd) sComment=memdupsafe(mx.sValue,uComment=mx.uValueLen,"IDC_PSHSAVE");
          }
          XMLReadClose(&mx);
        }
        HANDLE hFile=XMLWriteCreat(g_ofn.lpstrFile,"1.0",(pfr->InternalFlags&FRI_UNICODE)?"UTF-8":"Windows-1252");
        if (INVALID_HANDLE_VALUE!=hFile) {
          XMLWriteComment(hFile,0,sComment?sComment:"--Set Loaded--",uComment);
          XMLWriteLevelUp(hFile,0,"TextFX:Replace",(unsigned)-1);
#define PROCESSCHECK3(cs,flg,sflg,bt,ck,nm,ln) XMLWriteAttribute(hFile,"option:"nm,(unsigned)-1,(flg&bt)?"1":"0",1,'"');
#define PROCESSCHECK PROCESSCHECK3
          PROCESSCHECKSA /* Used here to generate XML option list */
#undef PROCESSCHECK
          char st[2];
          st[1]='\0';
          st[0]=ValidateWindowCharacter(GetDlgItem(hDlg,IDC_EDTSEQUENCE));
          XMLWriteAttribute(hFile,"option:SequenceCharacter",(unsigned)-1,st,(unsigned)-1,'"');
          st[0]=ValidateWindowCharacter(GetDlgItem(hDlg,IDC_EDTCOUNTER));
          XMLWriteAttribute(hFile,"option:CounterCharacter",(unsigned)-1,st,(unsigned)-1,'"');
          XMLWriteText(hFile,"",0,NULL);
          XMLWriteLevelUp(hFile,1,"FindText",(unsigned)-1);
          XMLWriteText(hFile,pfr->lpstrFindWhat,pfr->wFindWhatLen,"/"); // does this / mimic vi?
          XMLWriteLevelDownSame(hFile,"FindText",(unsigned)-1);
          XMLWriteLevelUp(hFile,1,"ReplaceText",(unsigned)-1);
          XMLWriteText(hFile,pfr->lpstrReplaceWith,pfr->wReplaceWithLen,"/");
          XMLWriteLevelDownSame(hFile,"ReplaceText",(unsigned)-1);
          XMLWriteLevelDown(hFile,0);
          XMLWriteLevelDownSame(hFile,"TextFX:Replace",(unsigned)-1);
          XMLWriteClose(hFile);
        }
        if (sComment) freesafe(sComment,"IDC_PSHSAVE");
      }
#else
      SetWindowTextA(GetDlgItem(hDlg,stc1),"This key doesn't work yet!");
#endif
      break;
    case IDC_PSHSWAP: {
        char *szBufRepl=NULL; size_t uBufReplSz=0,uBufReplLen;
        GetWindowTextUTF8arm(NULL,GetDlgItem(hDlg,IDC_EDTREPLACE),&szBufRepl,&uBufReplSz,&uBufReplLen); if (!szBufRepl) break;
        char *szBufFind=NULL; size_t uBufFindSz=0,uBufFindLen;
        GetWindowTextUTF8arm(NULL,GetDlgItem(hDlg,IDC_EDTFIND),&szBufFind,&uBufFindSz,&uBufFindLen);
        if (szBufFind) {
          SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTFIND),szBufRepl,uBufReplLen,-1);
          SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTREPLACE),szBufFind,uBufFindLen,-1);
          pfr->InternalFlags|=(FRI_DIRTYREPLACE|FRI_DIRTYFIND);
          freesafe(szBufFind,"FindReplaceDlgProc");
        }
        freesafe(szBufRepl,"FindReplaceDlgProc");
      } SetFocus((HWND)lParam); // putting text in IDC_EDTREPLACE is messing up the Focus
      return TRUE;
    case IDC_PSHREPLACEFIND:
    case IDC_PSHREPLACEREST:
      if (!ReplaceCheck(hDlg,pfr)) break;
    case IDC_PSHFIND:
    case IDC_PSHCOUNT:
      if (!FindCheck(hDlg,pfr)) break;
      if (pfr->InternalFlags&FRI_DIRTYCOLUMN) {
        char szCols[24];
        unsigned uLen=GetWindowTextA(GetDlgItem(hDlg,IDC_EDTCOLUMNS),szCols,sizeof(szCols));
        if (uLen<2) {
badcols:  pfr->uCol1=pfr->uCol2=0;
          SetWindowTextA(GetDlgItem(hDlg,IDC_EDTCOLUMNS),"#-#");
          pfr->InternalFlags&=~FRI_DIRTYCOLUMN; // undo what EN_CHANGE does
          if (uLen) {
            CheckFlags(hDlg,pfr);
            SetWindowTextA(GetDlgItem(hDlg,stc1),"Invalid columns. Example: '1-5' or '6-'");
            return TRUE;
          }
        } else {
          unsigned uIndex=0;
          int uCol1=atoi(strtokX(szCols,&uIndex,"- \t"));
          int uCol2=atoi(strtokX(szCols,&uIndex,"- \t"));
          if (uCol1<1 || (uCol2<uCol1 && uCol2)) goto badcols;
          pfr->uCol1=uCol1;
          pfr->uCol2=uCol2; //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("uCol1:%d uCol2:%d"),uCol1,uCol2),TEXT("???"),MB_OK);
        }
        pfr->InternalFlags&=~FRI_DIRTYCOLUMN;
        CheckFlags(hDlg,pfr);
      }
      {
        BOOL fSwitchDown;
        if ((GetKeyState(VK_SHIFT)&0x8000) && IsWindowEnabled(GetDlgItem(hDlg,IDC_RADUP))) {
          pfr->PersistentFlags |= FRP_UP;
          fSwitchDown=TRUE;
        } else fSwitchDown=FALSE;
        if (pfr->wFindWhatLen && (pfr->cbFindReplaceX(LOWORD(wParam),pfr) || (HIWORD(wParam)==IDC_EDTFIND && !(pfr->InternalFlags&FRI_NOTFOUND)))) DestroyWindow(hDlg);
        if (fSwitchDown /*&& IsWindowEnabled(GetDlgItem(hDlg,IDC_RADUP)*/) {
          CheckRadioButton(hDlg,IDC_RADUP,IDC_RADDOWN,IDC_RADDOWN);
          pfr->PersistentFlags &= ~FRP_UP;
          pfr->StorageFlags &= ~FRP_UP;
        }
      }
      return TRUE;
    case IDC_PSHRESTOREPOS: pfr->InternalFlags   |= FRI_RESTORE; // fall through to IDCANCEL
    case IDCANCEL:
idcancel:
      DestroyWindow(hDlg);
      return TRUE;
    case IDC_CMBFOLDERS:
      if (HIWORD(wParam)==CBN_SELENDOK) { // I wanted a feature where a path starting with a - couldn't be saved to but Most-Recent-Folder makes it too complicated
        /*unsigned uCursel=*/pfr->uPathListSelected=SENDMSGTOED((HWND)lParam,CB_GETCURSEL,0,0);
        /*char *szBuf=(char *)mallocsafe(SENDMSGTOED((HWND)lParam,CB_GETLBTEXTLEN,uCursel,0)+1,"FindReplaceDlgProc:IDC_CMBFOLDERS");
        if (szBuf) unsigned uTextLen=SENDMSGTOED((HWND)lParam,CB_GETLBTEXT,uCursel,szBuf);
        EnableWindow(GetDlgItem(hDlg,IDC_PSHSAVE),!szBuf || szBuf[0]!='-');
        freesafe(szBuf,"FindReplaceDlgProc:IDC_CMBFOLDERS");*/
        return TRUE;
      }
      break;
    case IDC_CMBFINDHISTW:
    case IDC_CMBFINDHISTA:
      if (HIWORD(wParam)==CBN_SELENDOK) {
        unsigned uCursel=SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
        if (uCursel) {
          pfr->InternalFlags |= FRI_BLOCKUPDATE;
          SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTFIND),g_FindHist.pszBuf[uCursel-1],g_FindHist.puBufLen[uCursel-1]-1,-1);
          pfr->InternalFlags &= ~FRI_BLOCKUPDATE;
          //pfr->InternalFlags|=FRI_DIRTYFIND;
          return TRUE;
        }
      }
      break;
    case IDC_CMBREPLHISTW:
    case IDC_CMBREPLHISTA:
      if (HIWORD(wParam)==CBN_SELENDOK) {
        unsigned uCursel=SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
        if (uCursel) {
          pfr->InternalFlags |= FRI_BLOCKUPDATE;
          SetWindowTextUTF8(pfr,GetDlgItem(hDlg,IDC_EDTREPLACE),g_ReplHist.pszBuf[uCursel-1],g_ReplHist.puBufLen[uCursel-1]-1,-1);
          pfr->InternalFlags &= ~FRI_BLOCKUPDATE;
          //pfr->InternalFlags|=FRI_DIRTYREPLACE;
          return TRUE;
        }
      }
      break;
    case IDC_CMBREGEX:
      if (HIWORD(wParam)==CBN_SELENDOK) {
        unsigned uCursel=SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
        if (uCursel) {
          unsigned uTextLen=SendMessage((HWND)lParam,CB_GETLBTEXTLEN,uCursel,0);
          char *szBuf=(char *)mallocsafe(uTextLen+1,"FindReplaceDlgProc"); if (!szBuf) break;
          uTextLen=SendMessage((HWND)lParam,CB_GETLBTEXT,uCursel,(LPARAM)szBuf);
          char *p=(char *)memchr(szBuf,':',uTextLen);
          if (p) {
            HWND hEdt=GetDlgItem(hDlg,memicmp(szBuf,"REPLACE ",8)?IDC_EDTFIND:IDC_EDTREPLACE);
            SendMessage(hEdt,SCI_REPLACESEL,0,(LPARAM)(p+2));
            SetFocus(hEdt);
            if (!(pfr->ScintillaFlags&SCFIND_REGEXP) && IsWindowEnabled(GetDlgItem(hDlg,IDC_CHXREGEX))) {
              pfr->ScintillaFlags|=SCFIND_REGEXP;
              PROCESSCHECK1(IDC_CHXREGEX,pfr->ScintillaFlags,pfr->SciStorageFlags,SCFIND_REGEXP,TRUE,"RegEx",5);
              CheckFlags(hDlg,pfr);
            }
          }
          freesafe(szBuf,"FindReplaceDlgProc");
          SendMessage((HWND)lParam,CB_SETCURSEL,0,0);
          return TRUE;
        }
      }
      break;
    case IDC_RADDOWN      : pfr->PersistentFlags &= ~FRP_UP; return TRUE;
    case IDC_RADUP        : pfr->PersistentFlags |= FRP_UP; SetWindowTextA(GetDlgItem(hDlg,stc1),"UP Shortcut: Shift+Find/Count/Replace"); return TRUE;
#define PROCESSCHECK2(cs,flg,sflg,bt,ck,nm,ln) \
    case cs:\
      if (SendMessage((HWND)lParam,BM_GETCHECK,0,0)==BST_CHECKED) flg |= (bt); else flg &= ~(bt);\
      if (ck) CheckFlags(hDlg,pfr);\
      return TRUE
#define PROCESSCHECK PROCESSCHECK2
    PROCESSCHECKSA PROCESSCHECKSB /* Used here to process case statements for checks */
#undef PROCESSCHECK
#undef PROCESSCHECKSA
#undef PROCESSCHECKSB
    }
    break;
  case WM_CLOSE: goto idcancel;
  }
  return FALSE ;
}

// gets the Scintilla results for iCmd1 and iCmd2 and puts the lowest into uLow and the higest into uHigh
// returns TRUE if the result for iCmd2 was less than iCmd1
static inline BOOL GetLowHigh(HWND hwndEditor,int iCmd1,int iCmd2,unsigned *uLow,unsigned *uHigh) {
  unsigned uCmd1=SENDMSGTOED(hwndEditor, iCmd1, 0, 0);
  unsigned uCmd2=SENDMSGTOED(hwndEditor, iCmd2, 0, 0);
  if (uCmd1<=uCmd2) {
    *uLow=uCmd1;
    *uHigh=uCmd2;
    return FALSE;
  } else {
    *uLow=uCmd2;
    *uHigh=uCmd1;
    return TRUE;
  }
}

#define XORLOGICAL(flag1,flag2) ((flag1)?1:0) ^ ((flag2)?1:0)
static BOOL PopFindFindText(LPFINDREPLACEX pfr,BOOL fReplacing) {
  if (!(pfr->InternalFlags&FRI_UNMARKED) || (pfr->InternalFlags&FRI_NOTFOUND)) {
    if (!(pfr->InternalFlags&FRI_UNMARKED) && (unsigned)SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONSTART, 0, 0)==pfr->ep1 && (unsigned)SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONEND, 0, 0)==pfr->ep2) {
      unsigned epGoPos;
      if (pfr->PersistentFlags&FRP_SELECTED) epGoPos=(pfr->PersistentFlags&FRP_UP)?pfr->ep2:pfr->ep1;
      else                                   epGoPos=(pfr->PersistentFlags&FRP_UP)?pfr->ep1:pfr->ep2;
      pfr->eprpTail=pfr->eprpHead=epGoPos;
      SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS, epGoPos, 0);
    } else pfr->eprpTail=pfr->eprpHead=SENDMSGTOED(pfr->hwndEditor, SCI_GETCURRENTPOS, 0, 0);
    pfr->InternalFlags|=FRI_UNMARKED; //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("ep1:%d ep2:%d %s %s"),pfr->ep1,pfr->ep2,(pfr->PersistentFlags&FRP_UP)?"up":"down",(pfr->PersistentFlags&FRP_SELECTED)?"selected":"not selected"),TEXT("???"),MB_OK);
    pfr->iLastLine=-1; //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("ep1:%d ep2:%d %s %s"),pfr->ep1,pfr->ep2,(pfr->PersistentFlags&FRP_UP)?"up":"down",(pfr->PersistentFlags&FRP_SELECTED)?"selected":"not selected"),TEXT("???"),MB_OK);
  }
  SENDMSGTOED(pfr->hwndEditor, SCI_SETSEARCHFLAGS, pfr->ScintillaFlags, 0);
  do {
    unsigned epTargStart,epTargEnd;
    if (pfr->InternalFlags&(FRI_INVISIBLE|FRI_INVISIBLEONESHOT)) {
      epTargStart=pfr->eprpHead;
      epTargEnd=pfr->eprpTail;
      if (epTargStart>epTargEnd) {
        epTargStart=epTargEnd;
        epTargEnd=pfr->eprpHead;
      }
    } else {
      epTargStart=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONSTART, 0, 0);  // trying to save one of these two makes the code too complicated
      epTargEnd=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONEND, 0, 0);
    } // always: epTargStart<epTargEnd
    if (pfr->PersistentFlags&FRP_UP) {
      unsigned sellen=epTargEnd-epTargStart;
      if ((pfr->PersistentFlags&FRP_RECURSIVE) && fReplacing && !(pfr->InternalFlags&FRI_INVISIBLEONESHOT)) epTargStart=epTargEnd;
      epTargEnd=((pfr->PersistentFlags&FRP_SELECTED) || ((pfr->PersistentFlags&FRP_WRAP) && (epTargStart>pfr->ep1 || (epTargStart==pfr->ep1 && sellen))))?pfr->ep1:0;
      if (epTargEnd>epTargStart) goto fail;
    } else {
      unsigned sellen=epTargEnd-epTargStart;
      if (!((pfr->PersistentFlags&FRP_RECURSIVE) && fReplacing && !(pfr->InternalFlags&FRI_INVISIBLEONESHOT)) )  epTargStart=epTargEnd;
      //epTargStart=epTargEnd;
      epTargEnd=((pfr->PersistentFlags&FRP_SELECTED) || ((pfr->PersistentFlags&FRP_WRAP) && (epTargStart<pfr->ep2 || (epTargStart==pfr->ep2 && sellen))))?pfr->ep2:SENDMSGTOED(pfr->hwndEditor, SCI_GETTEXTLENGTH, 0, 0);
      if (epTargEnd<epTargStart) goto fail;
    }
    pfr->InternalFlags|=FRI_INVISIBLEONESHOT; // for every "continue"
    SENDMSGTOED(pfr->hwndEditor, SCI_SETTARGETSTART,epTargStart, 0); //MessageBoxFree(pfr->hwndSelf,TEXT("epTargStart:%d epTargEnd:%d"),epTargStart,epTargEnd),TEXT("???"),MB_OK);
    SENDMSGTOED(pfr->hwndEditor, SCI_SETTARGETEND  ,epTargEnd, 0); //MessageBoxFree(pfr->hwndSelf,smprintf(,TEXT("epTargStart:%d epTargEnd:%d"),epTargStart,epTargEnd),TEXT("???"),MB_OK);
    if (epTargStart==epTargEnd || -1 == SENDMSGTOED(pfr->hwndEditor, SCI_SEARCHINTARGET,pfr->wFindWhatLen, pfr->lpstrFindWhat)) {
      if (pfr->PersistentFlags&FRP_WRAP) {
        if (pfr->PersistentFlags&FRP_UP) {
          if (epTargEnd<pfr->ep1) {
            pfr->eprpHead=pfr->eprpTail=SENDMSGTOED(pfr->hwndEditor, SCI_GETTEXTLENGTH, 0, 0)-1;
            //if (!(pfr->InternalFlags&(FRI_INVISIBLE|FRI_INVISIBLEONESHOT))) SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS, pfr->eprpHead, 0);
            continue;
          }
        } else {
          if (epTargEnd>pfr->ep2) {
            pfr->eprpHead=pfr->eprpTail=0;
            //if (!(pfr->InternalFlags&(FRI_INVISIBLE|FRI_INVISIBLEONESHOT))) SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS, 0, 0);
            continue;
          }
        }
      }
fail: pfr->eprpTail=pfr->eprpHead=epTargEnd;
      if (!(pfr->InternalFlags&FRI_INVISIBLE)) {
        if ((pfr->PersistentFlags&(FRP_WRAP|FRP_LINE1ST)) || pfr->uCol1) SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS,epTargEnd,0); // we shouldn't send this when counting
        SetWindowText(GetDlgItem(pfr->hwndSelf,stc1),"Not Found");
        if (!(pfr->InternalFlags&FRI_NOTFOUND)) MessageBeep(MB_ICONHAND);
      }
      //pfr->InternalFlags&=~FRI_UNMARKED; // This allows them to continue past the wrap anchor after a single failure
      pfr->iLastLine=-1; //MessageBox(0,"Not Found","???",MB_OK);
      pfr->InternalFlags|=FRI_NOTFOUND;
    } else {
      pfr->InternalFlags&=~FRI_NOTFOUND;
      pfr->eprpHead=SENDMSGTOED(pfr->hwndEditor, (pfr->PersistentFlags&FRP_UP)?SCI_GETTARGETSTART:SCI_GETTARGETEND,0,0);
      pfr->eprpTail=SENDMSGTOED(pfr->hwndEditor, (pfr->PersistentFlags&FRP_UP)?SCI_GETTARGETEND:SCI_GETTARGETSTART,0,0);
      if (pfr->PersistentFlags&FRP_LINE1ST) {
        int lnTail=SENDMSGTOED(pfr->hwndEditor, SCI_LINEFROMPOSITION,pfr->eprpTail,0);
        if (pfr->iLastLine==lnTail) continue;
        pfr->iLastLine=lnTail; //MessageBoxFree(pfr->hwndSelf,smprintf("lnTail:%u iLastLine:%u",lnTail,pfr->iLastLine),TEXT("???"),MB_OK);
      }
      if (pfr->PersistentFlags&FRP_VISIBLE) {
        int lnHead=SENDMSGTOED(pfr->hwndEditor, SCI_LINEFROMPOSITION,pfr->eprpHead,0);
        if (!SENDMSGTOED(pfr->hwndEditor, SCI_GETLINEVISIBLE,lnHead,0)) continue;
        int lnTail=SENDMSGTOED(pfr->hwndEditor, SCI_LINEFROMPOSITION,pfr->eprpTail,0);
        if (lnTail != lnHead && !SENDMSGTOED(pfr->hwndEditor, SCI_GETLINEVISIBLE,lnTail,0)) continue;
      }
      if (pfr->uCol1) {
        unsigned uCol1,uCol2;
        if (pfr->eprpHead<pfr->eprpTail) {
          uCol1=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->eprpHead,0)+1;
          uCol2=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->eprpTail,0);
        } else {
          uCol1=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->eprpTail,0)+1;
          uCol2=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->eprpHead,0);
        } //MessageBoxFree(pfr->hwndSelf,smprintf("uCol1:%u ->uCol1:%u uCol2:%u ->uCol2:%u",uCol1,pfr->uCol1,uCol2,pfr->uCol2),TEXT("???"),MB_OK);
        if (uCol1<pfr->uCol1 || (pfr->uCol2 && uCol2>pfr->uCol2)) continue;
      }
      if (!(pfr->InternalFlags&FRI_INVISIBLE)) {
        SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS,pfr->eprpTail,0);
        SENDMSGTOED(pfr->hwndEditor, SCI_SETSEL,pfr->eprpTail,pfr->eprpHead);
        SetWindowText(GetDlgItem(pfr->hwndSelf,stc1),"Found!");
      } // Order is only important for SCI_SETSEL, the next search/replace chooses its own order
    } //SetWindowTextFree(GetDlgItem(pfr->hwndSelf,stc1),smprintf("Start:%d End:%d tail:%u head:%u",epTargStart,epTargEnd,pfr->eprpTail,pfr->eprpHead));
    EnableFindButtons(pfr,pfr->hwndSelf,pfr->wFindWhatLen?TRUE:FALSE);
    pfr->InternalFlags&=~FRI_INVISIBLEONESHOT;
    break;
  } while(1); //MessageBox(0,"???","???",MB_OK);
  return (pfr->InternalFlags&FRI_NOTFOUND)?FALSE:TRUE;
}

// This will return a malloc()'d buffer char or wchar_t buffer depending on fUnicode
// while the buffer is always NUL padded, the length never includes it
// uExtra, uSize, uLen are always in character, typically 1 for char, and 2 for wchar_t
static void *strdupSCI_GetTargetTextAW(HWND hwndEditor,BOOL fUnicode,unsigned uExtra,unsigned *puSize,unsigned *puLen) {
  void *rv=NULL;
  struct TextRange tr;
  tr.chrg.cpMin=SENDMSGTOED(hwndEditor, SCI_GETTARGETSTART,0,0);
  tr.chrg.cpMax=SENDMSGTOED(hwndEditor, SCI_GETTARGETEND,0,0);
  unsigned uReplTextSzA=tr.chrg.cpMax-tr.chrg.cpMin+1+(fUnicode?0:uExtra);
  tr.lpstrText=(char *)mallocsafe(uReplTextSzA,"SCI_GetTextRangeAW");
  if (tr.lpstrText) {
    unsigned uReplTextLenA=SENDMSGTOED(hwndEditor, SCI_GETTEXTRANGE,0,&tr); // does not include \0
    if (fUnicode) {
      unsigned uReplTextSzW=UCS2FromUTF8(tr.lpstrText,uReplTextLenA+1,NULL,0,FALSE,NULL)+uExtra;
      wchar_t *sReplTextW=(wchar_t *)mallocsafe(uReplTextSzW*sizeof(wchar_t),"SCI_GetTextRangeAW");
      if (sReplTextW) {
        unsigned uReplTextLenW=UCS2FromUTF8(tr.lpstrText,uReplTextLenA+1,sReplTextW,uReplTextSzW,FALSE,NULL)-1; //MessageBoxFree(0,smprintf(TEXT("uReplTextSzW:%u uReplTextLenW:%u uReplTextSzA:%u uReplTextLenA:%u"),uReplTextSzW,uReplTextLenW,uReplTextSzA,uReplTextLenA),TEXT("???"),MB_OK);
        if (puSize) *puSize = uReplTextSzW;
        if (puLen) *puLen = uReplTextLenW;
        rv=(void *)sReplTextW;
      }
      freesafe(tr.lpstrText,"SCI_GetTextRangeAW");
    } else {
        if (puSize) *puSize = uReplTextSzA;
        if (puLen) *puLen = uReplTextLenA;
        rv=(void *)tr.lpstrText;
    }
  }
  return rv;
}

// returns the ANSI/UTF8 length of the replacement string (same as SCI_REPLACETARGET)
static int SCI_ReplaceTargetW(HWND hwndEditor,unsigned uTextLenW,wchar_t *sTextW) {
  int rv=0;
  if (uTextLenW == (unsigned)-1) uTextLenW=wcslen(sTextW);
  if (uTextLenW) {
    unsigned uTextLenA=UTF8FromUCS2(sTextW,uTextLenW,NULL,0,FALSE);
    char *sTextA=(char *)mallocsafe(uTextLenA,"SCI_ReplaceTargetW");
    if (sTextA) {
      uTextLenA=UTF8FromUCS2(sTextW,uTextLenW,sTextA,uTextLenA,FALSE);
      rv=SENDMSGTOED(hwndEditor, SCI_REPLACETARGET,uTextLenA,sTextA);
      freesafe(sTextA,"SCI_ReplaceTargetW");
    }
  } else {
      rv=SENDMSGTOED(hwndEditor, SCI_REPLACETARGET,-1,"");
  }
  return rv;
}

// returns 0 if replace did not occur
// 1 if replace occured but failed due to recursion restrictions
// 2 if replace occured and there was no failure
static int PopFindReplaceText(LPFINDREPLACEX pfr,BOOL fUnify) {
  unsigned epTargStart,epTargEnd;
  if ((pfr->InternalFlags&FRI_NOTFOUND)) {
    SetWindowText(GetDlgItem(pfr->hwndSelf,stc1),"Must Find first.");
    goto fail;
  }
  if (pfr->InternalFlags&FRI_INVISIBLE) {
    epTargStart=pfr->eprpHead;
    epTargEnd=pfr->eprpTail;
    if (epTargStart>epTargEnd) {
      epTargStart=epTargEnd;
      epTargEnd=pfr->eprpHead;
    }
  } else {
    epTargStart=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONSTART, 0, 0);
    epTargEnd=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONEND, 0, 0);
  } // always: epTargStart<epTargEnd
  if (epTargStart==epTargEnd) {
    SetWindowText(GetDlgItem(pfr->hwndSelf,stc1),"Replace requires selected text.");
fail:
    pfr->InternalFlags|=FRI_NOTFOUND;
    EnableFindButtons(pfr,pfr->hwndSelf,pfr->wFindWhatLen?TRUE:FALSE);
    MessageBeep(MB_ICONHAND);
    return 0;
  } else {
    SENDMSGTOED(pfr->hwndEditor, SCI_SETTARGETSTART,epTargStart, 0);
    SENDMSGTOED(pfr->hwndEditor, SCI_SETTARGETEND  ,epTargEnd, 0);
#define CASEMIXED 0 /* same as CASEUNKNOWN */
#define CASEUPPER 1
#define CASELOWER 2
    unsigned uFindCase=CASEMIXED;
    if (pfr->PersistentFlags&FRP_REPLCASE) { // identify the caseness of the find text
      unsigned vLen; void *vBuf=strdupSCI_GetTargetTextAW(pfr->hwndEditor,(pfr->InternalFlags&FRI_UNICODE),0,NULL,&vLen);
      if (vBuf) {
        unsigned uUpChars=0,uLowChars=0;
        if (pfr->InternalFlags&FRI_UNICODE) {
#define sFoundTextW ((wchar_t *)vBuf)
          if (CapsTablesWStart(sizeof(wchar_t))) {
            unsigned i; for(i=0; i<vLen; i++) {
              if (IsCharUpperXW(sFoundTextW[i])) uUpChars++;
              else if (IsCharLowerXW(sFoundTextW[i])) uLowChars++;
            }
            CapsTablesWStop(0);
          }
#undef sFoundTextW
        } else {
#define sFoundTextA ((char *)vBuf)
          unsigned i; for(i=0; i<vLen; i++) {
            if (IsCharUpperA(sFoundTextA[i])) uUpChars++;
            else if (IsCharLowerA(sFoundTextA[i])) uLowChars++;
          }
#undef sFoundTextA
        }
        if (uUpChars && !uLowChars) uFindCase=CASEUPPER;
        else if (uLowChars && !uUpChars) uFindCase=CASELOWER;
        freesafe(vBuf,"PopFindReplaceText");
      }
    }
    if (fUnify) SENDMSGTOED(pfr->hwndEditor, SCI_BEGINUNDOACTION,0,0); // no gotos or breaks between the UNDOACTIONs allowed
/**/SENDMSGTOED(pfr->hwndEditor, (pfr->ScintillaFlags&SCFIND_REGEXP)?SCI_REPLACETARGETRE:SCI_REPLACETARGET ,pfr->wReplaceWithLen, pfr->lpstrReplaceWith);
    if (uFindCase!=CASEMIXED) { // apply the caseness to the replaced text
      unsigned vLen; void *vBuf=strdupSCI_GetTargetTextAW(pfr->hwndEditor,(pfr->InternalFlags&FRI_UNICODE),0,NULL,&vLen);
      if (vBuf) {
        if (pfr->InternalFlags&FRI_UNICODE) {
#define sReplTextW ((wchar_t *)vBuf)
          if (CapsTablesWStart(sizeof(wchar_t))) {
            unsigned i; for(i=0; i<vLen; i++) sReplTextW[i]=(uFindCase==CASEUPPER)?CharUpperXW(sReplTextW[i]):CharLowerXW(sReplTextW[i]);
            SCI_ReplaceTargetW(pfr->hwndEditor,vLen,sReplTextW);
            CapsTablesWStop(0);
          }
#undef sReplTextW
        } else {
#define sReplTextA ((char *)vBuf)
          unsigned i; for(i=0; i<vLen; i++) sReplTextA[i]=(uFindCase==CASEUPPER)?CharUpperXA(sReplTextA[i]):CharLowerXA(sReplTextA[i]);
          SENDMSGTOED(pfr->hwndEditor, SCI_REPLACETARGET,vLen,sReplTextA);
#undef sReplTextA
        }
        freesafe(vBuf,"PopFindReplaceText");
      }
    }
    //if (pfr->cCounter) {
    //}
    int cchReplacedx=SENDMSGTOED(pfr->hwndEditor, SCI_GETTARGETEND,0,0)-epTargEnd;
    //unsigned ep1Old=pfr->ep1,ep2Old=pfr->ep2; // DEBUG line
    if (pfr->ep1>epTargEnd) pfr->ep1 += cchReplacedx;
    if (pfr->ep2>epTargEnd) pfr->ep2 += cchReplacedx;
    epTargEnd+=cchReplacedx;
    if (XORLOGICAL(pfr->PersistentFlags&FRP_UP,pfr->PersistentFlags&FRP_RECURSIVE)) {
      pfr->eprpHead=epTargStart;
      pfr->eprpTail=epTargEnd;
    } else {
      pfr->eprpHead=epTargEnd;
      pfr->eprpTail=epTargStart;
    } // Order is only important for SCI_SETSEL, the next search/replace chooses its own order
    if (!(pfr->InternalFlags&FRI_INVISIBLE)) SENDMSGTOED(pfr->hwndEditor, SCI_SETSEL,pfr->eprpTail,pfr->eprpHead); //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("cchReplacedx:%d ep1Old:%u ep2Old:%u ep1New:%u ep2New:%u targstart:%u targend:%u"),cchReplacedx,ep1Old,ep2Old,pfr->ep1,pfr->ep2,pfr->eprpHead,pfr->eprpTail),TEXT("???"),MB_OK);
    if (fUnify) SENDMSGTOED(pfr->hwndEditor, SCI_ENDUNDOACTION,0,0);
    return(((pfr->PersistentFlags&FRP_RECURSIVE) && cchReplacedx>=0)?1:2);
  }
}

/* Scintilla is not well behaved for Dialog Boxes. It doesn't honor ES_WANTRETURN       and it consumes TAB and Shift-TAB
 * We will duplicate the Dialog functionality for these keys */
static HWND g_hDlg;
static LRESULT APIENTRY ScintillaSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,WNDPROC wpOrigProc,SHORT wFromID) {
  switch(uMsg) {
  case WM_KEYDOWN:
    switch(wParam) {
    case VK_ESCAPE: PostMessage(g_hDlg,WM_COMMAND,MAKEWPARAM(IDCANCEL,wFromID),(LPARAM)NULL); break;
    case VK_TAB   : SetFocus(GetNextDlgTabItem(g_hDlg,GetFocus(),(GetKeyState (VK_SHIFT)&0x8000)?TRUE:FALSE)); break;
    case VK_RETURN:
      if (IsWindowEnabled(GetDlgItem(g_hDlg,IDC_PSHFIND)) && !(GetKeyState(VK_SHIFT)&0x8000) && !(GetKeyState (VK_CONTROL)&0x8000) && !(GetKeyState (VK_MENU)&0x8000)) PostMessage(g_hDlg,WM_COMMAND,MAKEWPARAM(IDC_PSHFIND,wFromID),(LPARAM)NULL);
      break;
    }
    break;
  }
  return CallWindowProc(wpOrigProc, hwnd, uMsg, wParam, lParam);
}

static WNDPROC g_wpScintillaOrigProc1;
static LRESULT APIENTRY ScintillaSubclassProc1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ScintillaSubclassProc(hwnd, uMsg, wParam, lParam,g_wpScintillaOrigProc1,IDC_EDTFIND);
}
static WNDPROC g_wpScintillaOrigProc2;
static LRESULT APIENTRY ScintillaSubclassProc2(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ScintillaSubclassProc(hwnd, uMsg, wParam, lParam,g_wpScintillaOrigProc2,IDC_EDTREPLACE);
}

static int __cdecl PopFindReplaceCallback(unsigned uCommand,LPFINDREPLACEX pVoid) {
  int uReplaceLimit;
  LPFINDREPLACEX pfr=(LPFINDREPLACEX)pVoid;
  switch(uCommand) {
  case FRC_DIALOGCREATE: {
    pfr->InternalFlags=FRI_NOTFOUND;
    pfr->StorageFlags=0;
    pfr->SciStorageFlags=0;
    pfr->uCol1=pfr->uCol2=0;
    if (GetLowHigh(pfr->hwndEditor,SCI_GETANCHOR,SCI_GETCURRENTPOS,&pfr->ep1,&pfr->ep2)) pfr->InternalFlags |= FRI_LESS;
    if (!pfr->ep1 || pfr->ep2==(unsigned)SENDMSGTOED(pfr->hwndEditor, SCI_GETTEXTLENGTH, 0, 0)) pfr->InternalFlags|=FRI_BOFEOF;
    if (pfr->ep1 == pfr->ep2) {
      EnableWindow(GetDlgItem(pfr->hwndSelf,IDC_CHXSELECTED),FALSE);
      SendDlgItemMessage (pfr->hwndSelf,IDC_CHXSELECTED,BM_SETCHECK,0,0);
      if (pfr->PersistentFlags&FRP_SELECTED) {pfr->PersistentFlags&= ~FRP_SELECTED; pfr->StorageFlags|=FRP_SELECTED;}
    } else {
      EnableWindow(GetDlgItem(pfr->hwndSelf,IDC_CHXSELECTED),TRUE);
      if (pfr->PersistentFlags&FRP_SELECTED) CheckFlags(pfr->hwndSelf,pfr);
    }
    g_hDlg=pfr->hwndSelf;
    g_wpScintillaOrigProc1=(WNDPROC)SetWindowLongPtr(GetDlgItem(pfr->hwndSelf,IDC_EDTFIND),GWLP_WNDPROC,(LONG_PTR)ScintillaSubclassProc1);
    if ((pfr->PersistentFlags&FRP_AUTOGRABFIND) && !(pfr->PersistentFlags&FRC_FINDINCREMENTAL) ) {
      unsigned uSellen=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELTEXT, 0, NULL)-1; // NUL byte not included
      if (!uSellen) { // get current word
        unsigned epCurpos=SENDMSGTOED(pfr->hwndEditor, SCI_GETCURRENTPOS, 0, 0);
        unsigned epWord1=SENDMSGTOED(pfr->hwndEditor, SCI_WORDSTARTPOSITION, epCurpos, TRUE);
        unsigned epWord2=SENDMSGTOED(pfr->hwndEditor, SCI_WORDENDPOSITION, epCurpos, TRUE);
        uSellen=epWord2-epWord1; // NUL not included here
        //MessageBoxFree(pfr->hwndSelf,smprintf(TEXT("epCurpos:%u epWord1:%u epWord2:%u uSellen:%u"),epCurpos,epWord1,epWord2,uSellen),TEXT("???"),MB_OK);
        if (uSellen) {
		  tsize_t findWhatSz = pfr->wFindWhatSz;
          armreallocsafe(&pfr->lpstrFindWhat,&findWhatSz,uSellen+1,ARMSTRATEGY_MAINTAIN,0,"PopFindReplaceCallback");
		  pfr->wFindWhatSz = findWhatSz;
          if (pfr->lpstrFindWhat) {
            struct TextRange tr;
            tr.lpstrText=pfr->lpstrFindWhat; tr.chrg.cpMin=epWord1; tr.chrg.cpMax=epWord2;
            pfr->wFindWhatLen=SENDMSGTOED(pfr->hwndEditor, SCI_GETTEXTRANGE, 0, &tr);
          }
        }
      } else { // get current selection
        if ((pfr->iSelIsRect=SENDMSGTOED(pfr->hwndEditor, SCI_GETSELECTIONMODE, 0, 0))==SC_SEL_RECTANGLE) {
          int uCol1,uCol2;
          uCol1=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->ep1,0);
          uCol2=SENDMSGTOED(pfr->hwndEditor, SCI_GETCOLUMN,pfr->ep2,0);
          if (uCol1>uCol2) {int temp=uCol1; uCol1=uCol2; uCol2=temp;}
          pfr->uCol1=uCol1++;
          pfr->uCol2=uCol2;
          SetWindowTextFree(GetDlgItem(pfr->hwndSelf,IDC_EDTCOLUMNS),smprintf("%u-%u columns",uCol1,uCol2));
        }
		tsize_t findWhatSz = pfr->wFindWhatSz;
		armreallocsafe(&pfr->lpstrFindWhat, &findWhatSz, uSellen + 1, ARMSTRATEGY_MAINTAIN, 0, "PopFindReplaceCallback");
		pfr->wFindWhatSz = findWhatSz;
		if (pfr->lpstrFindWhat) pfr->wFindWhatLen = SENDMSGTOED(pfr->hwndEditor, SCI_GETSELTEXT, 0, pfr->lpstrFindWhat) - 1;
      }
    }
    int iCodePage=SENDMSGTOED(pfr->hwndEditor,SCI_GETCODEPAGE,0,0);
    SetWindowTextUTF8(pfr,GetDlgItem(pfr->hwndSelf,IDC_EDTFIND),pfr->lpstrFindWhat,pfr->wFindWhatLen,iCodePage);
    g_wpScintillaOrigProc2=(WNDPROC)SetWindowLongPtr(GetDlgItem(pfr->hwndSelf,IDC_EDTREPLACE),GWLP_WNDPROC,(LONG_PTR)ScintillaSubclassProc2);
    SetWindowTextUTF8(pfr,GetDlgItem(pfr->hwndSelf,IDC_EDTREPLACE),pfr->lpstrReplaceWith,pfr->wReplaceWithLen,iCodePage);
    SetWindowTextFree(GetDlgItem(pfr->hwndSelf,stc1),smprintf("^I=Tab ^M=Return (%s)",(pfr->InternalFlags&FRI_UNICODE)?"UNICODE":"ANSI"));
    } break;
  case FRC_REPLACEALL:
    uReplaceLimit=0;
replacesome:
    SENDMSGTOED(pfr->hwndEditor, SCI_BEGINUNDOACTION, 0, 0);
    int iReplaceResult;
    if ((iReplaceResult=PopFindReplaceText(pfr,FALSE))!=0) {
      uReplaceLimit--;
      unsigned cReplaces=1;
      pfr->eprpHead=SENDMSGTOED(pfr->hwndEditor, SCI_GETANCHOR, 0, 0);
      pfr->eprpTail=SENDMSGTOED(pfr->hwndEditor, SCI_GETCURRENTPOS, 0, 0); // order doesn't matter
      pfr->InternalFlags|=FRI_INVISIBLE;
      while(PopFindFindText(pfr,TRUE) && uReplaceLimit && iReplaceResult!=1) {iReplaceResult=PopFindReplaceText(pfr,FALSE); cReplaces++; uReplaceLimit--;}
      SetWindowTextFree(GetDlgItem(pfr->hwndSelf,stc1),smprintf("%u %s",cReplaces,(iReplaceResult==1)?"Repl (recursion requires smaller replace)":"Bulk Replacements"));
      pfr->InternalFlags&=~FRI_INVISIBLE;
      SENDMSGTOED(pfr->hwndEditor, SCI_SETSEL, pfr->eprpHead, pfr->eprpTail);
    }
    SENDMSGTOED(pfr->hwndEditor, SCI_ENDUNDOACTION, 0, 0);
    break;
  case FRC_REPLACE: {
      char buf[32];
      GetWindowText(GetDlgItem(pfr->hwndSelf,IDC_EDTREPLACECT),buf,sizeof(buf));
      uReplaceLimit=atoi(buf);
    }
    if (uReplaceLimit<1) {/*uReplaceLimit=1;*/ SetWindowText(GetDlgItem(pfr->hwndSelf,IDC_EDTREPLACECT),"1"); }
    else if (uReplaceLimit>1) goto replacesome;
    if (PopFindReplaceText(pfr,TRUE)<1) break; // else fall through
  case FRC_FINDNEXT:
    PopFindFindText(pfr,uCommand==FRC_REPLACE); // recursive find is not permitted without a replace
    break;
  case FRC_FINDCOUNT: {
      unsigned epAnchor=pfr->eprpHead=SENDMSGTOED(pfr->hwndEditor, SCI_GETANCHOR, 0, 0);
      unsigned epCurpos=pfr->eprpTail=SENDMSGTOED(pfr->hwndEditor, SCI_GETCURRENTPOS, 0, 0);
      unsigned uFlags=pfr->InternalFlags;
      pfr->InternalFlags|=FRI_INVISIBLE;
      unsigned cFinds=0;
      while(PopFindFindText(pfr,FALSE)) cFinds++;
      pfr->InternalFlags=uFlags;
      SENDMSGTOED(pfr->hwndEditor, SCI_GOTOPOS, epAnchor, epAnchor);
      if (epAnchor != epCurpos) PopFindFindText(pfr,FALSE); // must refind to ensure that any regex replace will be right
      SetWindowTextFree(GetDlgItem(pfr->hwndSelf,stc1),smprintf(TEXT("%u found"),cFinds));
    } break;
  case FRC_DIALOGTERM: { // canceling the subclass is not necessary
    if (pfr->InternalFlags & FRI_RESTORE) {
      unsigned epAnchor,epCurpos;
      if (pfr->InternalFlags&FRI_LESS) {epAnchor=pfr->ep2; epCurpos=pfr->ep1;}
      else                             {epAnchor=pfr->ep1; epCurpos=pfr->ep2;}
      SENDMSGTOED(pfr->hwndEditor, SCI_SETANCHOR,epAnchor,0);
      SENDMSGTOED(pfr->hwndEditor, SCI_SETSELECTIONMODE,pfr->iSelIsRect,0);
      SENDMSGTOED(pfr->hwndEditor, SCI_SETCURRENTPOS,epCurpos,0);
    }
    pfr->ScintillaFlags |=pfr->SciStorageFlags;
    pfr->PersistentFlags|=pfr->StorageFlags;
    pfr->SciStorageFlags=0;
    pfr->InternalFlags=0;
    pfr->StorageFlags=0; // just for fun
        RECT      rc;
    GetWindowRect(pfr->hwndSelf, &rc);
    /*pfr->xDlg=rc.left;
    pfr->cxDlg=rc.right-rc.left;
    pfr->yDlg=rc.top;
    pfr->cyDlg=rc.bottom-rc.top;*/
    pfr->cxScreen = GetSystemMetrics(SM_CXSCREEN);
    pfr->cyScreen = GetSystemMetrics(SM_CYSCREEN);
    } break;
  }
  return 0;
}

#define PopFindValidFind() (g_fr.hwndOwner && g_fr.hwndEditor && g_fr.wFindWhatLen && !IsBadReadPtr(g_fr.lpstrFindWhat,g_fr.wFindWhatLen) && g_fr.lpstrFindWhat[0] != '\0')

static FINDREPLACEX g_fr;

// if cbFindReplaceX is not specified, the built in one will be used
void PopFindReplaceDlg(HWND hwndOwner,HWND hwndEditor,HINSTANCE hInstance,CBFINDREPLACEX cbFindReplaceX,BOOL fFindAgain) {
  g_fr.cbFindReplaceX   = cbFindReplaceX?cbFindReplaceX:PopFindReplaceCallback;
  if (fFindAgain && PopFindValidFind()) {
    g_fr.cbFindReplaceX(FRC_FINDNEXT,&g_fr);
  } else if (g_fr.hwndSelf) {
    SetForegroundWindow(g_fr.hwndSelf);
  } else {
    g_fr.hwndOwner        = hwndOwner ;
    g_fr.hwndEditor       = hwndEditor ;
    g_fr.hwndSelf=(HWND)SENDMSGTOED(hwndOwner,NPPM_MODELESSDIALOG,MODELESSDIALOGADD,CreateDialogParamA(hInstance,"REPLACEDLG1",hwndOwner,FindReplaceDlgProc,(LPARAM)&g_fr));
    //if (!g_fr.hwndSelf) MessageBox(hwndOwner,"Create Failed","???",MB_ICONSTOP);
  }
}

void PopFindReplaceDlgDestroy(void) {
  if (g_fr.hwndSelf) DestroyWindow(g_fr.hwndSelf);
}

void PopFindReplaceDlgINI(BOOL fStop,/*const char *szPluginPath,*/const char *szINIPath) {
  if (!fStop) {
    if (!g_fr.szPathList) g_fr.uPathListSz=256;
    g_fr.uPathListLen=GetPrivateProfileStringarm("FindReplace","RepositoryFolders","|",&g_fr.szPathList,&g_fr.uPathListSz,szINIPath);
    if (g_fr.szPathList) {
      if (!strcmp(g_fr.szPathList,"|")) {
        g_fr.uPathListLen=0;
        strcpyarmsafe(&g_fr.szPathList,&g_fr.uPathListSz,&g_fr.uPathListLen,/*"-"*/"TextFX Original:%CSIDLX_TEXTFXDATA%\\ReplaceSets;Most Recent Folder:;My Documents:%CSIDL_PERSONAL%","PopFindReplaceDlgINI");
        if (g_fr.szPathList) WritePrivateProfileString("FindReplace","RepositoryFolders",g_fr.szPathList,szINIPath);
      }
      XlatPathEnvVarsarm(&g_fr.szPathList,&g_fr.uPathListSz,&g_fr.uPathListLen);
      //MessageBoxFree(0,smprintf("Buf:%u Len:%u\r\n%s",g_fr.uPathListSz,g_fr.uPathListLen,g_fr.szPathList),"???", MB_OK|MB_ICONWARNING);
    }
    g_fr.uPathListSelected=GetPrivateProfileInt("FindReplace","RepositorySelected",0,szINIPath);
    // It is not desirable to check that each path is valid because that could cause startup delays for Notepad++
  } else {
    WritePrivateProfileStringFree("FindReplace"     ,"RepositorySelected"         ,smprintf("%u",g_fr.uPathListSelected),szINIPath);
  }
}

void PopFindReplaceDlgInit(BOOL stop/*,const char *szPluginPath*/) {
  if (!stop) {
    INITCOMMONCONTROLSEX icx;
    icx.dwSize=sizeof(INITCOMMONCONTROLSEX);
    icx.dwICC=ICC_UPDOWN_CLASS|ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icx);
    memset(&g_fr,0,sizeof(g_fr));
    g_fr.PersistentFlags=FRP_SELECTED|FRP_WRAP|FRP_AUTOGRABFIND;
    g_fr.ScintillaFlags=SCFIND_POSIX;
    g_ofn.lpstrFile[0]='\0';
    g_ofn.lpstrFileTitle[0]='\0';
    char *szPath=NULL; unsigned uPathSz=0;
    NPPGetSpecialFolderLocationarm(CSIDLX_TEXTFXDATA,NULL,&szPath,&uPathSz,NULL,"ReplaceSets\\");
    if ((szPath/*(=smprintfpath("%s%?\\%s%?\\%s",szPluginPath,SUPPORT_PATH,"ReplaceSets\\")*/)) {
      strncpymem(g_szInit,NELEM(g_szInit),szPath,(unsigned)-1);
      freesafe(szPath,"PopFindReplaceDlgInit");
    }
  } else {
    if (g_fr.lpstrFindWhat) {freesafe(g_fr.lpstrFindWhat,"PopFindReplaceDlgInit"); g_fr.lpstrFindWhat=NULL;}
    if (g_fr.lpstrReplaceWith) {freesafe(g_fr.lpstrReplaceWith,"PopFindReplaceDlgInit"); g_fr.lpstrReplaceWith=NULL;}
    if (g_fr.szPathList) {freesafe(g_fr.szPathList,"PopFindReplaceDlgInit"); g_fr.szPathList=NULL;}
  }
  HistoryInit(&g_FindHist,stop);
  HistoryInit(&g_ReplHist,stop);
}
#ifdef __cplusplus
}
#endif

#endif

