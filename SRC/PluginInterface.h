//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#ifdef NPP_UNICODE
#define NPPTEXT(quote)	L##quote
#define NPPCHAR			wchar_t
#else
#define NPPTEXT(quote)	quote
#define NPPCHAR			char
#endif

#include <windows.h>
#include "Scintilla.h"
#include "Notepad_plus_msgs.h"

const int nbChar = 64;

typedef const TCHAR * (__cdecl * PFUNCGETNAME)();

struct NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
};

#define QSORT_FCMP __cdecl

typedef void (__cdecl * PFUNCSETINFO)(NppData);
//typedef void (__cdecl * PFUNCPLUGINCMD)();
typedef void (__cdecl * PBENOTIFIED)(SCNotification *);
typedef LRESULT (__cdecl * PMESSAGEPROC)(UINT Message, WPARAM wParam, LPARAM lParam);


struct ShortcutKey {
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	UCHAR _key;
};

#define PFUNCPLUGINCMD void __cdecl /* Some compilers cannot use the typedef in every case. This define works to alter function declarations with all compilers */
#if !defined(__WATCOMC__) || defined(__cplusplus)
typedef void (__cdecl PFUNCPLUGINCMD_MSC)(void); /* MSC is unable to declare variables without a typedef. Compilers other than Watcom prefer this method also but often work with warnings with the #define. */
#else
#define PFUNCPLUGINCMD_MSC PFUNCPLUGINCMD /* Watcom C does not allow conversion between typedefs and nontypedefs whether or not they are identical. */
#endif

struct FuncItem {
	NPPCHAR _itemName[nbChar];
	//PFUNCPLUGINCMD _pFunc;
	PFUNCPLUGINCMD_MSC *_pFunc;
	int _cmdID;
	bool _init2Check;
	ShortcutKey *_pShKey;
};

typedef FuncItem * (__cdecl * PFUNCGETFUNCSARRAY)(int *);

// You should implement (or define an empty function body) those functions which are called by Notepad++ plugin manager
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const NPPCHAR * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode();
#endif //UNICODE

#endif //PLUGININTERFACE_H
