/*******************************************************************************
 *** $Id$
 ***
 *** User initialisation/deinitinialisation code for the python.library
 ***
 */

/*
** Project Includes
*/

#define _LARGEFILE64_SOURCE

#include "libuser.h"


/*
** System Includes
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>


/*
** Public Function
*/

//+ __restore_r13
/*! __restore_r13()
** This function must preserve all registers except r13
*/
asm("\n"
"    .section \".text\"                                     \n"
"    .align 2                                               \n"
"    .type __restore_r13, @function                         \n"
"__restore_r13:                                             \n"
"    lwz 13, 36(3)                                          \n"
"    blr                                                    \n"
"__end__restore_r13:                                        \n"
"    .size __restore_r13, __end__restore_r13 - __restore_r13\n");
//-

extern unsigned long __get_handle(int fd);
extern void __seterrno(void);

/********************************************/
/*** Python library specialized Init/Term ***/

//+ UserLibOpen
SAVEDS ULONG UserLibOpen(pPythonLibrary_t pLibBase)
{
    pPyMorphOS_Exported_t ex;

    /* Setup some critical globals */
    PythonBase = pLibBase;
    DOSBase = PythonBase->MyDOSBase;
    SysBase = PythonBase->MySysBase;
    UtilityBase = PythonBase->MyUtilityBase;

    PythonBase->PythonGVars = NULL;
    ex = &PythonBase->Exported;

    /* Exporting some symbols for .pym dynamic modules (loaded with libdl so they don't share data) */
    ex->ex___get_handle = __get_handle;
    ex->ex_open = open;
    ex->ex_close = close;
    ex->ex_dup = dup;
    ex->ex_dup2 = dup2;
    ex->ex_fcntl = fcntl;
    ex->ex_fopen = fopen;
    ex->ex_fclose = fclose;
    ex->ex_fflush = fflush;
    ex->ex_fdopen = fdopen;
    ex->ex___seterrno = __seterrno;
    ex->ex___srget = __srget;
    ex->ex_setlocale = setlocale;
    ex->ex_atexit = atexit;
    ex->ex_tmpfile = tmpfile;
    ex->ex_mkstemp = mkstemp;
    ex->ex_freopen = freopen;
    ex->ex_remove = remove;
    ex->ex_rename = rename;
    ex->ex_getcwd = getcwd;
    ex->ex_chdir = chdir;
    ex->ex_stat = stat;
    ex->ex_access = access;
    ex->ex_fstat = fstat;
    ex->ex_chown = chown;
    ex->ex_mkdir = mkdir;
    ex->ex_poserr = poserr;
    ex->ex_write = write;
    ex->ex_read = read;
    ex->ex_lseek = lseek;
    ex->ex_lseek64 = lseek64;
    ex->ex_ftruncate = ftruncate;
    ex->ex_ftruncate64 = ftruncate64;
    ex->ex_fstat64 = fstat64;
    ex->ex_stat64 = stat64;
    ex->ex_chmod = chmod;
    ex->ex_fchown = fchown;
    ex->ex_fchmod = fchmod;

    return TRUE;
}
//-
//+ UserLibClose
SAVEDS void UserLibClose( pPythonLibrary_t pLibBase )
{
    ;
}
//-
