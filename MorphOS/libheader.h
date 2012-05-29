/*******************************************************************************
 *** $Id$
 ***
 *** Morphos Library Header.
 ***
 *******************************************************************************
 */

#ifndef LIBHEADER_H
#define LIBHEADER_H

/*
** System Includes
*/

#include <stdio.h>

#include <exec/initializers.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <emul/emulinterface.h>

#include <exec/io.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>


/*
** Public Macros and Definitions
*/

#ifndef SAVEDS
#define SAVEDS __saveds
#endif

#define BASE_IGNORE_SIZE (sizeof(struct SignalSemaphore))

#ifdef PYTHON_LIBTABLE
#define LIBPROTO(name) extern void _t_##name(void) __saveds
#define LIBDECL(name) (APTR) _t_##name,
#endif


/*
** Public Types and Structures
*/

typedef struct PyMorphOS_Exported
{
    APTR ex___get_handle;
    APTR ex_open;
    APTR ex_close;
    APTR ex_dup;
    APTR ex_dup2;
    APTR ex_fcntl;
    APTR ex_mempool;
    APTR ex_stdin;
    APTR ex_stdout;
    APTR ex_stderr;
    APTR ex_fopen;
    APTR ex_fclose;
    APTR ex_fflush;
    APTR ex_fdopen;
    APTR ex___seterrno;
    APTR ex___srget;
    APTR ex_setlocale;
    APTR ex_LocaleBase;
    APTR ex_atexit;
    APTR ex_tmpfile;
    APTR ex_mkstemp;
    APTR ex_freopen;
    APTR ex_remove;
    APTR ex_rename;
    APTR ex_getcwd;
    APTR ex_chdir;
    APTR ex_stat;
    APTR ex_access;
    APTR ex_fstat;
    APTR ex_chown;
    APTR ex_mkdir;
    APTR ex_poserr;
    APTR ex_write;
    APTR ex_read;
    APTR ex_lseek;
    APTR ex_lseek64;
    APTR ex_ftruncate;
    APTR ex_ftruncate64;
    APTR ex_fstat64;
    APTR ex_stat64;
    APTR ex_chmod;
    APTR ex_fchown;
    APTR ex_fchmod;
    APTR ex_malloc;
    APTR ex_free;
    APTR ex_exit;
    APTR ex__exit;
} PyMorphOS_Exported_t, *pPyMorphOS_Exported_t;

typedef struct PythonTaskNode
{
    struct MinNode  Node;
    struct Task     *Task;
} PythonTaskNode_t, *pPythonTaskNode_t;

typedef struct PythonLibrary
{
    struct Library          Library;
    UBYTE                   Alloc;
    UBYTE                   Pad;
    APTR                    DataSeg;    /* DON'T CHANGE POSITION */
    ULONG                   DataSize;

    struct PythonLibrary    *Parent;
    BPTR                    SegList;

    struct ExecBase         *MySysBase;
    struct DosLibrary       *MyDOSBase;
    struct Library          *MyUtilityBase;

    union
    {
        struct MinList      TaskList;
        PythonTaskNode_t    TaskNode;
    } TaskContext; /* Permits to given same base pointer when a task call twice the OpenLibrary() */

    APTR                    MallocMemPool;  /* used by emul/malloc.c */
    APTR                    PythonGVars;    /* All exported globals used internaly by python.
                                             * One copy per base.
                                             */
    PyMorphOS_Exported_t    Exported;
    /* following fields will not be included in child (see BASE_IGNORE_SIZE) */

    struct SignalSemaphore  Semaphore;

} PythonLibrary_t, *pPythonLibrary_t;

/*
** Public Function Prototypes
*/

extern ULONG            LibReserved( void );
extern struct Library * LibInit( APTR, BPTR, struct ExecBase * );
extern BPTR             LibExpunge(void );
extern BPTR             LibClose( void );
extern struct Library * LibOpen( void );


/*
** Public Variables
*/

extern int              __datadata_relocs(void);

#ifndef PROTO_PYTHONLIB_H
extern pPythonLibrary_t PythonBase;
#endif

#endif /* LIBHEADER_H */
