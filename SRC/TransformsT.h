#define TFUNCTION(xyzzy) xyzzy##A
#define TCHARDYNAMIC char
#include "TransformsT.cpp"
#undef TFUNCTION
#undef TCHARDYNAMIC
#define TFUNCTION(xyzzy) xyzzy##W
#define TCHARDYNAMIC WCHAR
#include "TransformsT.cpp"
#undef TFUNCTION
#undef TCHARDYNAMIC

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif
