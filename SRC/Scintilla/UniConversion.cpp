// Scintilla source code edit control
/** @file UniConversion.cxx
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// (C) Copyright 2005-2006 Chris Severance under GNU GPL (Scintilla is also (C) under GNU GPL)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include "UniConversion.h"
// http://www.joelonsoftware.com/articles/Unicode.html

// This code has been hand optimized to produce the best assembly code in MinGW/GCC
// The Length counters are hand optimized to be extremely fast by minimizing branch prediction failures and minimizing JMP instructions.
// Compilers often use SAR/SAL of 16 bit values in 32 bit registers so the sign never matters.

// WideCharToMultiByte(CP_UTF8,
// converts a UCS-2 string to a UTF-8 string. call with len=0 to obtain length
unsigned UTF8FromUCS2(const wchar_t *uptro, unsigned int tlen, char *putf,unsigned int len,int fBigEndian) {
  if (!len) {
    if (!fBigEndian) {
      const wchar_t *uend=uptro+tlen;
      unsigned int len = 0;
      for (; uptro<uend; uptro++) {
        len++;
        if (*uptro >= 0x0080) len++;
        if (*uptro >= 0x0800) len++;
      }
      return len;
    } else {
      const wchar_t *uend=uptro+tlen;
      unsigned int len = 0;
      for (; uptro<uend; uptro++) {
        len++;
        if (*uptro & 0x80FF) len++;
        if (*uptro & 0x00F8) len++;
      }
      return len;
    }
  } else if (!fBigEndian) {
    const wchar_t *uptr=uptro,*uend=uptro+tlen;
    unsigned char *uputf=(unsigned char *)putf,*upend=uputf+len;
    for (; uptr<uend && uputf<upend; uptr++) {
      wchar_t uch = *uptr;
      if (uch < 0x0080) {
        *uputf++ = (unsigned char)uch;
      } else if (uch < 0x0800) {    // 00000xxxvvnnnnnn
        if (uputf>=upend-1) break; // 110xxxvv 10nnnnnn
        *uputf++ = (unsigned char)(0xC0 | ((uch >> 6) /* & 0x1F */)); //0x80+0x40
        *uputf++ = (unsigned char)(0x80 |  (uch & 0x3f));
      } else {                     // qqqqxxxxvvnnnnnn
        if (uputf>=upend-2) break; // 1110qqqq 10xxxxvv 10nnnnnn
        *uputf++ = (unsigned char)(0xE0 | ((uch >> 12) & 0xF)); //0x80+0x40+0x20
        *uputf++ = (unsigned char)(0x80 | ((uch >> 6) & 0x3f));
        *uputf++ = (unsigned char)(0x80 |  (uch & 0x3f));
      }
    }
    return((char *)uputf-putf);
  } else {
    const wchar_t *uptr=uptro,*uend=uptro+tlen;
    unsigned char *uputf=(unsigned char *)putf,*upend=uputf+len;
    for (; uptr<uend && uputf<upend; uptr++) {
      wchar_t uch = *uptr;
      if (!(uch & 0x80FF)) { // 0nnnnnnn00000000 -> nnnnnnnn
        *uputf++ = (unsigned char)(uch>>8);
      } else if (!(uch & 0x00F8)) {// vvnnnnnn00000xxx
        if (uputf>=upend-1) break; // 110xxxvv 10nnnnnn
        *uputf++ = (unsigned char)(0xC0 | ((uch >>14) & 0x03) | ((uch << 2) & 0x1c)); //0x80+0x40
        *uputf++ = (unsigned char)(0x80 | ((uch >> 8) & 0x3f));
      } else {                     // vvnnnnnnqqqqxxxx
        if (uputf>=upend-2) break; // 1110qqqq 10xxxxvv 10nnnnnn
        *uputf++ = (unsigned char)(0xE0 | ((uch >> 4) & 0x0f)); //0x80+0x40+0x20
        *uputf++ = (unsigned char)(0x80 | ((uch >>14) & 0x03) | ((uch << 2) & 0x3c));
        *uputf++ = (unsigned char)(0x80 | ((uch >> 8) & 0x3f));
      }
    }
    return((char *)uputf-putf);
  }
}

// produces valid UTF-8 from possibly invalid UTF-8
unsigned UTF8Validated(const char *s, unsigned int len, char *putf, unsigned int plen,unsigned *cchUnused) {
  const unsigned char *sx=(unsigned char *)s,*endx=sx+len;
  unsigned char *uputf=(unsigned char *)putf,*upend=uputf+plen;
  unsigned dx;
  wchar_t uch;
  while (sx<endx) { // no need for Big/Little Endian here, and Little Endian is better code
    if (*sx < 0x80) {
      uch = *sx;
      dx=1;
    } else if (*sx < (0x80 + 0x40 + 0x20)) { // 110xxxvv 10nnnnnn
      if (sx>=endx-1) break;                 // 00000xxxvvnnnnnn
      uch = ((*sx & 0x1F) << 6) | (sx[1] & 0x3F);
      dx=2;
    } else {                 // 1110qqqq 10xxxxvv 10nnnnnn
      if (sx>=endx-2) break; // qqqqxxxxvvnnnnnn
      uch = ((*sx & 0xF) << 12) | ((sx[1] & 0x3F) << 6) | (sx[2] & 0x3F);
      dx=3;
    }
    if (uch < 0x0080) {
      *uputf++ = (unsigned char)uch;
    } else if (uch < 0x0800) {    // 00000xxxvvnnnnnn
      if (uputf>=upend-1) break; // 110xxxvv 10nnnnnn
      *uputf++ = (unsigned char)(0xC0 | ((uch >> 6) /* & 0x1F */)); //0x80+0x40
      *uputf++ = (unsigned char)(0x80 |  (uch & 0x3f));
    } else {                     // qqqqxxxxvvnnnnnn
      if (uputf>=upend-2) break; // 1110qqqq 10xxxxvv 10nnnnnn
      *uputf++ = (unsigned char)(0xE0 | ((uch >> 12) & 0xF)); //0x80+0x40+0x20
      *uputf++ = (unsigned char)(0x80 | ((uch >> 6) & 0x3f));
      *uputf++ = (unsigned char)(0x80 |  (uch & 0x3f));
    }
    sx+=dx;
  }
  if (cchUnused) *cchUnused=endx-sx;
  return((char *)uputf-putf);
}

// MultiByteToWideChar(CP_UTF8,0,
// converts a UTF-8 string to a UCS-2 string. call with tlen=0 for length
// Unlike Scintilla, this code does not depend on the obligitory 0 bit separating the leading bits and the data bits so I use 0x3F instead of 0x7F
// This will allow illegal UTF-8 to convert to UCS-2 properly, as would be expected for an editor.
unsigned int UCS2FromUTF8(const char *s, unsigned int len, wchar_t *tbufo, unsigned int tlen,int fBigEndian,unsigned *cchUnused) {
  if (!tlen) {
    const unsigned char *sx=(unsigned char *)s,*endx=sx+len;
    unsigned int ulen=0;
    for (;sx<endx;sx++) if (*sx < 0x80 || *sx > (0x80+0x40)) ulen++;
    return ulen;
  } else if (!fBigEndian) {
    const unsigned char *sx=(unsigned char *)s,*endx=sx+len;
    wchar_t *tbuf=tbufo,*endt=tbuf+tlen;
    while (sx<endx && tbuf<endt) {
      if (*sx < 0x80) {
        *tbuf++ = *sx;
        sx++;
      } else if (*sx < (0x80 + 0x40 + 0x20)) { // 110xxxvv 10nnnnnn
        if (sx>=endx-1) break;                 // 00000xxxvvnnnnnn
        *tbuf++ = ((*sx & 0x1F) << 6) | (sx[1] & 0x3F);
        sx+=2;
      } else {                 // 1110qqqq 10xxxxvv 10nnnnnn
        if (sx>=endx-2) break; // qqqqxxxxvvnnnnnn
        *tbuf++ = ((*sx & 0xF) << 12) | ((sx[1] & 0x3F) << 6) | (sx[2] & 0x3F);
        sx+=3;
      }
    }
    if (cchUnused) *cchUnused=endx-sx;
    return(tbuf-tbufo);
  } else {
    const unsigned char *sx=(unsigned char *)s,*endx=sx+len;
    wchar_t *tbuf=tbufo,*endt=tbuf+tlen;
    while (sx<endx && tbuf<endt) {
      if (*sx < 0x80) {
        *tbuf++ = (*sx)<<8;
        sx++;
      } else if (*sx < (0x80 + 0x40 + 0x20)) { // 110xxxvv 10nnnnnn
        if (sx>=endx-1) break;                 // vvnnnnnn00000xxx
        *tbuf++ = ((*sx & 0x3) << 14) | ((*sx & 0x1C) >> 2) | ((sx[1] & 0x3F)<<8);
        sx+=2;
      } else {                 // 1110qqqq 10xxxxvv 10nnnnnn
        if (sx>=endx-2) break; // vvnnnnnnqqqqxxxx
        *tbuf++ = ((*sx & 0xF) << 4) | ((sx[1] & 0x3) << 14) | ((sx[1] & 0x3C) >> 2) | ((sx[2] & 0x3F)<<8);
        sx+=3;
      }
    }
    if (cchUnused) *cchUnused=endx-sx;
    return(tbuf-tbufo);
  }
}

// http://en.wikipedia.org/wiki/Utf-8
// return 0 for neither
// return 1 for ANSI
// return 2 for UTF-8 16-bit
int isUTF8_16(const char *s, unsigned int len,unsigned *cchUnused) {
  int rv=2;
  int ASCII7only=1;
  const unsigned char *sx=(unsigned char *)s,*endx=sx+len;
  while (sx<endx) {
    if (!*sx) {  // For detection, we'll say that NUL means not UTF8
      ASCII7only=0;
      rv=0;
      break;
    } else if (*sx < 0x80) { // 0nnnnnnn If the byte's first hex code begins with 0-7, it is an ASCII character.
      sx++;
    } else if (*sx < (0x80 + 0x40)) { // 10nnnnnn 8 through B cannot be first hex codes
      ASCII7only=0;
      rv=0;
      break;
    } else if (*sx < (0x80 + 0x40 + 0x20)) { // 110xxxvv 10nnnnnn  If it begins with C or D, it is an 11 bit character
      ASCII7only=0;
      if (sx>=endx-1) break;
      if (!(*sx & 0x1F) || (sx[1]&(0x80+0x40)) != 0x80) {rv=0; break;}
      sx+=2;
    } else if (*sx < (0x80 + 0x40 + 0x20 + 0x10)) { // 1110qqqq 10xxxxvv 10nnnnnn If it begins with E, it is 16 bit
      ASCII7only=0;
      if (sx>=endx-2) break;
      if (!(*sx & 0xF) || (sx[1]&(0x80+0x40)) != 0x80 || (sx[2]&(0x80+0x40)) != 0x80) {rv=0; break;}
      sx+=3;
    } else { /* more than 16 bits are not allowed here */
      ASCII7only=0;
      rv=0;
      break;
    }
  }
  if (cchUnused) *cchUnused=endx-sx;
  return(ASCII7only?1:rv);
}
