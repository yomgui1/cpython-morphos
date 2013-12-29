#define _STDIO_H_ 1 /* exclude stdio.h inclusion */

#include "libheader.h"

extern pPythonLibrary_t PythonBase;

#define ALIAS(f, n) int f(void) __attribute__(( alias(#n) ));
#define DECLEXP(f) \
    int __PyMOS_Wrap_##f (void) { return ((int (*)(void))PythonBase->Exported.ex_##f)(); } \
    ALIAS(f, __PyMOS_Wrap_##f)

DECLEXP(fcntl);
DECLEXP(__get_handle);
DECLEXP(open);
DECLEXP(close);
DECLEXP(dup);
DECLEXP(dup2);
DECLEXP(fopen);
DECLEXP(fclose);
DECLEXP(fflush);
DECLEXP(fdopen);
DECLEXP(__seterrno);
DECLEXP(__srget);
DECLEXP(setlocale);
DECLEXP(atexit);
DECLEXP(tmpfile);
DECLEXP(mkstemp);
DECLEXP(freopen);
DECLEXP(remove);
DECLEXP(rename);
DECLEXP(getcwd);
DECLEXP(chdir);
DECLEXP(stat);
DECLEXP(access);
DECLEXP(fstat);
DECLEXP(chown);
DECLEXP(mkdir);
DECLEXP(poserr);
DECLEXP(write);
DECLEXP(read);
DECLEXP(lseek);
DECLEXP(lseek64);
DECLEXP(ftruncate);
DECLEXP(ftruncate64);
DECLEXP(fstat64);
DECLEXP(stat64);
DECLEXP(chmod);
DECLEXP(fchown);
DECLEXP(fchmod);

ALIAS(__open,open);
ALIAS(__close,close);
ALIAS(__dup,dup);
ALIAS(__dup2,dup2);
ALIAS(unlink,remove);
ALIAS(rmdir,remove);
ALIAS(lstat,stat);
ALIAS(__write,write);
ALIAS(__lseek,lseek);
ALIAS(lstat64,stat64);
