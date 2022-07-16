#ifndef SCFIND_MATCHCASE
#include "../scintilla.h"
#endif

#ifndef IDC_PSHREPLACEFIND
#include "popfindr.h"
#endif

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

/* Some of the original CommDlg.h::FindText() is well done but the notification, flags, and structures
   are malconceived and improperly implemented. We'll preserve the right and fix the wrong! */
#define FRC_DIALOGCREATE                 0     // switch-case will ensure no duplicate values here
#define FRC_DIALOGTERM                   IDCANCEL
#define FRC_FINDINCREMENTAL              IDCANCEL+1
#define FRC_FINDNEXT                     IDC_PSHFIND
#define FRC_FINDCOUNT                    IDC_PSHCOUNT
#define FRC_REPLACE                      IDC_PSHREPLACEFIND /* Replace and find again */
#define FRC_REPLACEALL                   IDC_PSHREPLACEREST

#define FRP_UP                           0x0001u /* 1 if the user wants and is allowed to go up, 0 for down */
#define FRP_INCREMENTAL                  0x0002u /* 1 for an incremental search */
#define FRP_SELECTED                     0x0004u /* 1 if the user is requesting and is allowd to search only selected */
#define FRP_WRAP                         0x0008u /* 1 if the user wants find/replace to wrap */
#define FRP_RECURSIVE                    0x0010u /* 1 if the user wants a recursive find */
#define FRP_AUTOGRABFIND                 0x0020u /* 1 if the user wants the find text to be drawn from the current selection or nearby word */
#define FRP_LINE1ST                      0x0040u /* 1 if only the first occurance on a line should be found/replaced  */
#define FRP_VISIBLE                      0x0080u /* 1 if start and end of find text must be visible */
#define FRP_REPLCASE                     0x0100u /* 1 if replacement text needs to match the case of the text being replaced */
#define FRP_MAXIMIZED                    0x0200u /* 1 if the dialog was last maximized */
// taken care of entirely by regular expressions

#define FRI_UNMARKED                     0x00000001u /* 0 if there hasn't been a first search to clear the selection and establish RegEx replace params */
#define FRI_NOTFOUND                     0x00000002u /* 1 if the last search resulted in a find */
#define FRI_INVISIBLE                    0x00000004u /* 1 if the search proceeds without screen updates */
#define FRI_INVISIBLEONESHOT             0x00000008u /* 1 if the search should be handled invisibly just one time */
#define FRI_BOFEOF                       0x00000010u /* 1 if the search was started at bof() or eof() (no wrapping allowed) */
#define FRI_RESTORE                      0x00000020u /* 1 if the user asks for the original position to be restored on exit */
#define FRI_LESS                         0x00000040u /* 1 if curpos<anchor, restores the selection correctly upon exit */
#define FRI_DIRTYFIND                    0x00000080u /* 1 if the find text is no longer the same as lpstrFindWhat */
#define FRI_DIRTYREPLACE                 0x00000100u /* 1 if the replace text is no longer the same as lpstrReplaceWith */
#define FRI_UNICODE                      0x00000200u /* 1 if the F&R controls are UNICODE */
#define FRI_BLOCKUPDATE                  0x00000400u /* 1 if edit:WM_NOTIFY should not reset the history positions to top */
#define FRI_DIRTYCOLUMN                  0x00000800u /* 1 if the columns text has been changed */

// http://www-128.ibm.com/developerworks/power/library/pa-ctypes1/index.html
struct tagREPCOUNTER {
  char szId[4];
  int iStart;  // also stores the current value
  int iIncrement;
  int iWidth;
};

struct tagFINDREPLACEX;
typedef int (__cdecl *CBFINDREPLACEX)(unsigned,struct tagFINDREPLACEX *);
typedef struct tagFINDREPLACEX {
  HWND         hwndSelf,hwndOwner,hwndEditor;
  unsigned     ScintillaFlags;     // Persistant flags that specifically pertain to Scintilla SCI_SEARCHINTARGET from Scintilla.h
  unsigned     SciStorageFlags;    // Scintilla flags are stored here when disabled
  WORD         PersistentFlags;    // User selected flags that are maintained while the dialog does not exist
  WORD         StorageFlags;       // PersistentFlags are stored here when disabled from restrictions
  unsigned     InternalFlags;      // Flags that are zeroed out each time the Dialog is created
  LPSTR        lpstrFindWhat;      // ptr. to search string, ANSI or UTF-8, always \0 terminated (length+1)
  LPSTR        lpstrReplaceWith;   // ptr. to replace string, ANSI or UTF-8, always \0 terminated (length+1)
  size_t       wFindWhatSz;        // malloc()'d length in BYTES of find buffer size
  size_t       wFindWhatLen;       // text length in BYTES of find buffer text (excludes \0)
  size_t       wReplaceWithSz;     // malloc()'d length in BYTES of replace buffer size
  size_t       wReplaceWithLen;    // text length in BYTES of replace buffer text (excludes \0)
  CBFINDREPLACEX cbFindReplaceX;   // Callback function to perform editor specific functionality
  unsigned     ep1,ep2;            // editor positions Curpos/Anchor when Replace dialog box is created
  unsigned     eprpHead,eprpTail;  // position of last find or replace
  unsigned     uCol1,uCol2;        // required columns 1-n in which find must be entirely located, or uCol1==0 to disable (uCol2==0 means end of line)
  int          iLastLine;          // the last line a find was made on if this option is in use, -1 for no line matched
  int cxScreen,cyScreen;           // screen size when dialog was last shown
  RECT rcDlg;                      // RECT coords when dialog was last shown (includes flag FRP_MAXIMIZED)
  int iSelIsRect;                  // stores the selection mode
  char *szPathList;                // The list of Repository paths
  unsigned uPathListSz,uPathListLen;
  unsigned uPathListSelected;      // The last szPathList selected
  char cSequence,cCounter,cUnused1,cUnused2;
  struct tagREPCOUNTER *RepCounters;
  //LPVOID       *lpvUser;         // If we were implementing this as a generic control we would provide a pointer like this one for caller managed data
} FINDREPLACEX,*LPFINDREPLACEX;

EXTERNC void PopFindReplaceDlg  (HWND,HWND,HINSTANCE,CBFINDREPLACEX,BOOL);
//EXTERNC CALLBACK PopFindReplaceCallback(unsigned,LPVOID); // return 0:preserved Dlg; 1:destroy Dlg
EXTERNC void PopFindReplaceDlgDestroy(void);
EXTERNC void PopFindReplaceDlgINI(BOOL fStop,/*const char *szPluginPath,*/const char *szINIPath);
EXTERNC void PopFindReplaceDlgInit(BOOL fStop/*,const char *szPluginPath*/);

#define SENDMSGTOED(edit,mesg,wpm,lpm) SendMessage(edit,mesg,(WPARAM)(wpm),(LPARAM)(lpm))
