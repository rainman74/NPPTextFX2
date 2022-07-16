// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef UNICONVERSION_H
#define UNICONVERSION_H

#ifdef __cplusplus
extern "C" {
#endif
unsigned UTF8FromUCS2(const wchar_t *,unsigned int,char *,unsigned int,int);
unsigned UTF8Validated(const char *, unsigned int, char *, unsigned int,unsigned *);
unsigned int UCS2FromUTF8(const char *,unsigned int,wchar_t *,unsigned int,int,unsigned int *);
int isUTF8_16(const char *, unsigned int,unsigned *);
#ifdef __cplusplus
}
#endif

#endif
