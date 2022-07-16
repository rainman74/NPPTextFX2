#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include /*..*/"NPPTextFX.h"
#include "MicroXML.h"

#ifdef __BORLANDC__
#pragma warn -pia /* Borland-C: possibly incorrect assignment */
#endif

#if defined(__POCC__) /* Pelles C */ || defined(_MSC_VER)
#define itoa _itoa
#define memicmp _memicmp
#if defined(__POCC__)
#pragma warn(disable:2216 2209) /* Return value from function is never used; Unreachable code removed */
#endif
#endif

//http://www.w3.org/TR/2004/REC-xml-20040204/

// 8 bit characters such as ANSI and UTF-8 are supported
// 16 bit characters aren't
// It's not possible for the XML parser to do any conversion since it cannot know
// what the source and destinations formats are.

// The other XML parsers are coded in C++ with no thought to performance or sensibility so I had to rewrite
// This implements high speed serial XML parsing only which is the only kind we need and what all XML parsing
// is based on.

// TinyXML provides the impossible, the entire XML file as an object
// expat is at least in C but the call structure is far too complicated.

// This won't read OOo documents because OOo writes text after close tags and I cannot find
// a standard that allows this.

EXTERNC void XMLXlatFromEntity(struct MICROXML *pmx) {
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
        *cRes=(unsigned char)ulVal;
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
      memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,(char *)cRes,sizeof(cRes),"XMLXlatFromEntity"); if (!sValueConverted) goto fail;
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
EXTERNC char *strdupXMLXlatToEntity(const char *sValue,unsigned uValueLen,unsigned *puDestLen) {
  char *sValueConverted=NULL;
  unsigned uValueConvertedLen=0;
  if (uValueLen) {
    unsigned uValueConvertedSz=0;
    const char *sValueEnd=sValue+uValueLen;
    const char *sValueP;
    unsigned quick[256/(sizeof(unsigned)*8)];
    if (sValue<sValueEnd && *sValue<32) { // convert the first control character to a numeric entity
      char *szNumCode=smprintf("&#%u;",*sValue);
      if (szNumCode) {
        memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,szNumCode,(unsigned)-1,"strdupXMLXlatToEntity");
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
fail: ;
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
EXTERNC int XMLBufMin(struct MICROXML *pmx,unsigned uMinSize,char *psFrom) {
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

EXTERNC int XMLRead(struct MICROXML *pmx) {
  if (pmx->_iState<=MX_DONE) return pmx->_iState;
  pmx->uLevel += pmx->_iLevelDeferred;
  pmx->_iLevelDeferred=0;
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
          pmx->_iLevelDeferred= -1;
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
          pmx->_iLevelDeferred=1;
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
        //pmx->sValue=(pmx->uValueLen=psRunner-pmx->_psBufBegin)?pmx->_psBufBegin:"";
        pmx->uValueLen=psRunner-pmx->_psBufBegin;
        pmx->sValue=pmx->_psBufBegin;
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
          //psQuote+=dx;
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

EXTERNC void XMLReadClose(struct MICROXML *pmx) {
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
EXTERNC BOOL XMLReadOpen(struct MICROXML *pmx,const char *fn,DWORD oflags,DWORD shflags) {
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
    pmx->iUsed=TRUE;
    return TRUE;
fail:
    XMLReadClose(pmx);
  }
  return FALSE;
}

static char g_sTabs[]="\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
EXTERNC void XMLWriteClose(HANDLE fo) {
  _writeX(fo,"\n",1);
  _closeX(fo);
}
EXTERNC void XMLWriteLevelDown(HANDLE fo,unsigned uLevel) {
  _writeX(fo,"\n",1);
  while(uLevel) {
    unsigned uToWrite=(uLevel>sizeof(g_sTabs)-1)?sizeof(g_sTabs)-1:uLevel;
    _writeX(fo,g_sTabs,uToWrite); uLevel-=uToWrite;
  }
}

EXTERNC void XMLWriteLevelUp(HANDLE fo,unsigned uLevel,const char *sTag,unsigned uTagLen) {
  if (uTagLen == (unsigned)-1) uTagLen=strlen(sTag);
  XMLWriteLevelDown(fo,uLevel);
  _writeX(fo,"<",1);
  _writeX(fo,sTag,uTagLen);
}

EXTERNC void XMLWriteLevelDownSame(HANDLE fo,const char *sTag,unsigned uTagLen) {
  if (sTag) {
    if (uTagLen == (unsigned)-1) uTagLen=strlen(sTag);
    _writeX(fo,"</",2);
    _writeX(fo,sTag,uTagLen);
    _writeX(fo,">",1);
  } else _writeX(fo,"/>",2);
}

EXTERNC void XMLWriteAttribute(HANDLE fo,const char *sAttribute,unsigned uAttributeLen,const char *sValue,unsigned uValueLen,unsigned cQuote) {
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
EXTERNC void XMLWriteText(HANDLE fo,const char *sValue,unsigned uValueLen,const char *szPfxSfx) {
  char *sValueConverted;
  unsigned uValueConvertedLen;
  unsigned uPfxSfxLen=0;
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
EXTERNC void XMLWriteComment(HANDLE fo,unsigned uLevel,const char *sValue,unsigned uValueLen) {
  if (uValueLen == (unsigned)-1) uValueLen=strlen(sValue);
  XMLWriteLevelDown(fo,uLevel);
  _writeX(fo,"<!",2);
  _writeX(fo,sValue,uValueLen);
  _writeX(fo,">",1);
}

EXTERNC HANDLE XMLWriteCreat(const char *szfn,const char *szVersion,const char *szEncoding) {
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
EXTERNC void XMLTest(const char *fn1,const char *fn2) {
  struct MICROXML mx;
  int i;
  //int res;
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
      XMLWriteLevelDown(fo2,mx.uLevel-1);
#ifdef __MSDOS__
      printf("\n%*s",mx.uLevel-1,"");
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

#ifdef __BORLANDC__
#pragma warn -par /* Borland-C: Parameter 'ident' is never used	 */
#endif
#if NPPDEBUG
EXTERNC void XMLDiscardNotify(const char *msg,struct MICROXML *pmx,unsigned uLevel,int iXMLRead) {
            MessageBoxFree(0/*pfr->hwndSelf*/,smprintf(TEXT("%s: XL %u Discard: %u"),msg,pmx->uLevel,iXMLRead),TEXT("Find & Replace"),MB_OK);
}
#endif

// Discard all read items above uLevel
EXTERNC int XMLReadLevel(struct MICROXML *pmx,unsigned uLevel,int iXMLReadOld) {
          int iXMLRead; // FAIL, DONE, DOWN, and TEXT:length=0 are not expected to be used
#if NPPDEBUG
          if (!pmx->iUsed && iXMLReadOld>MX_DOWNLIMIT && !(iXMLReadOld==MX_TEXT && pmx->uValueConvertedLen==0)) XMLDiscardNotify("Unused",pmx,uLevel,iXMLReadOld);
#endif
          while(1) {
            if ((iXMLRead=XMLRead(pmx))<=MX_DONE || pmx->uLevel==uLevel) break;
#if NPPDEBUG
            XMLDiscardNotify("Level  ",pmx,uLevel,iXMLRead);
#endif
          }
          pmx->iUsed=FALSE;
          return iXMLRead;
}
