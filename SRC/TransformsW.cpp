#define UNICODE
#define _UNICODE
#define TFUNCTION(xyzzy) xyzzy##W
#define TFUNCTIONCODE
#define TCHARDYNAMIC WCHAR
//#define TCHARPROMOTETOUNSIGNED(x) ((unsigned)(x))
//#define TCHARDEMOTETOUNSIGNED(x) ((TCHARDYNAMIC)(x))
#include "TransformsT.cpp"
