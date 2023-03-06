#ifdef PLAT_WIN
#define NPPWINDOWS 1 /* These 0 or 1 definitions allow us to use the constants in C code for the macros */
#else
#define NPPWINDOWS 0
#endif
#ifndef NPPDEBUG
#define NPPDEBUG 1 /* set to 1 to turn on string bounds checkers and error boxes */
#endif

/* Here we enable or disable code imported from other projects. You may need this because:
   The import source code is no longer useful, available, or legal to use in certain markets.
   The import source code originally used was not provided and it's too hard to figure out how to retrofit replacement source code.
   The imported function generates unacceptably large compiled code that can be eliminated for some platforms.
   The import function cannot be implemented on some platforms.
   The import is already implemented by another program for some platforms.
   The import function is causing problems and removing it would help pinpoint the problem source.
   The import function does not compile and run with the desired compiler.
   You want to know what parts of the code were affected by the addition of a particular import.
   You want to know how much code an import adds.
   You want to disable a feature that may later be needed

   Any import functions disabled should also have their #include in the .RC file disable
   */
#ifndef ENABLE_TIDYDLL /* This allows a -D to set this for us */
#define ENABLE_TIDYDLL 0
#endif
#define ENABLE_PUSHPOP 0
#define ENABLE_FINDREPLACE 1
#define ENABLE_WrapFriendlyHomeEnd 0
#define ENABLE_AutoShowMatchline 0
#define X86 1