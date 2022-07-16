struct MICROXML {
//public: vars do not start with _
  unsigned uLevel;
  BOOL iUsed;
  char *sValue; unsigned uValueLen; // (almost) always points into _sBuf
  char *sValueConverted; unsigned uValueConvertedSz,uValueConvertedLen; // NULL or a malloc()'d string with the converted contents of sValue
  char *sAttribute; unsigned uAttributeLen; // always points into _sBuf
  char *sPath;
  unsigned uPathSz;
  unsigned uPathLen;
  char *sLeaf; // always points into sXMLPath
  unsigned uLeafLen;
  unsigned cQuote;
  char sVersion[16];
  char sEncoding[16];
  //void *vUserData;    // caller defined value
  //unsigned uUserData; // caller defined value
//private: vars start with _
  char *_psBufBegin,*_psBufEnd; // these point within _sBuf
  char *_sBuf;
  unsigned _uSbufSz;
  HANDLE _hFileHandle;
  int _iState;
  int _iLevelDeferred;
};

/* both states and return values */
#define MX_FAIL      0 /* XML is invalid */
#define MX_DONE      1 /* end of file */
#define MX_LEVELDOWN 2 /* returns sValue=exiting sLeaf (sLeaf is now the previous leaf), returns NULL for self closing leaf (sAttribute=NULL) */
#define MX_LEVELDOWNSAME 3 /* same as above but indicates specially that there were no sub-tags */
#define MX_DOWNLIMIT 3 /* values <= this mean the end of the current level */
#define MX_LEVELUP   4 /* returns sValue=sLeaf=newly entered leaf */
#define MX_ATTRIBUTE 5 /* returns sAttribute='sValue' */
#define MX_TEXT      6 /* returns sValue (sAttribute=NULL) */
#define MX_COMMENT   7 /* returns sValue (sAttribute=NULL) */

#define MX_ATTRIBUTESPECIAL 255 /* This is a state allowing <?xml ... ?>, this is never returned as a value */

EXTERNC int XMLRead(struct MICROXML *pmx);
EXTERNC BOOL XMLReadOpen(struct MICROXML *pmx,const char *fn,DWORD oflags,DWORD shflags);
EXTERNC void XMLReadClose(struct MICROXML *pmx);

EXTERNC void XMLWriteClose(HANDLE fo);
EXTERNC void XMLWriteLevelDown(HANDLE fo,unsigned uLevel);
EXTERNC void XMLWriteLevelUp(HANDLE fo,unsigned uLevel,const char *sTag,unsigned uTagLen);
EXTERNC void XMLWriteLevelDownSame(HANDLE fo,const char *sTag,unsigned uTagLen);
EXTERNC void XMLWriteAttribute(HANDLE fo,const char *sAttribute,unsigned uAttributeLen,const char *sValue,unsigned uValueLen,unsigned cQuote);
EXTERNC void XMLWriteText(HANDLE fo,const char *sValue,unsigned uValueLen,const char *szPfxSfx);
EXTERNC void XMLWriteComment(HANDLE fo,unsigned uLevel,const char *sValue,unsigned uValueLen);
EXTERNC HANDLE XMLWriteCreat(const char *szfn,const char *szVersion,const char *szEncoding);
#if NPPDEBUG
EXTERNC void XMLTest(const char *fn1,const char *fn2);
#endif
EXTERNC int XMLReadLevel(struct MICROXML *pmx,unsigned uLevel,int iXMLReadOld);
