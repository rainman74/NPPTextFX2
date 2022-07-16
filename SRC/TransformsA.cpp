#undef UNICODE
#undef _UNICODE
#define TFUNCTION(xyzzy) xyzzy##A
#define TFUNCTIONCODE
#define TCHARDYNAMIC char
//#define TCHARPROMOTETOUNSIGNED(x) ((unsigned)(TCHARDYNAMIC)(x))
//#define TCHARDEMOTETOUNSIGNED(x) ((CHAR)(TCHARDYNAMIC)(x))
#include "TransformsT.cpp"
