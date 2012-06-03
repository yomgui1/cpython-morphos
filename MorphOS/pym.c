/*******************************************************************************
 * $Id:$
 *
 */

/*
** Project Includes
*/

#include "Python.h"

#include "morphos.h"
#include "mosdebug.h"
#include "libraries/python2_gvars.h"


/*
** System Includes
*/

#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <constructor.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
 


/*
** Types and Structures
*/

typedef int (*ct_func)(void);
typedef void (*dt_func)(void);

struct CTDT
{
    int (*fp)(void);
    long priority;
};

struct FuncSeg
{
    ULONG size;
    struct FuncSeg *next;
};


/*
** Prototypes
*/

static struct CTDT *sort_ctdt(struct CTDT **last);    
static void* null_malloc(size_t s);
static void null_free(void *p);


/*
** Public Variables
*/

struct ExecBase *SysBase = NULL;
struct DosLibrary *DOSBase = NULL;
struct Library *TimerBase = NULL;
struct Library *DynLoadBase = NULL;
pPythonLibrary_t PythonBase;
struct PyMorphOS_GVar_STRUCT __pym_GVars;
jmp_buf libnix_jmpbuf;
BOOL libnix_call;
STRPTR _ProgramName = "";

/* Required libnix startup globals */
int ThisRequiresConstructorHandling = 1;
APTR libnix_LocaleBase __attribute__ ((weak)) = NULL;

static FILE *old_stdio[3];
static void* (*Wrapper_malloc)(size_t s) = null_malloc;
static void (*Wrapper_free)(void *p) = null_free;

/* De/constructor stuff */
extern const void (* const __ctrslist[])(void);
extern const void (* const __dtrslist[])(void);
extern const struct CTDT __ctdtlist[];
static struct CTDT *ctdt, *last_ctdt;

/*================================================================================*/
/* Constructors/Destructors handling functions------------------------------------*/

//+ ANSI de/constructor handler
static void CallFuncArray(const void (* const FuncArray[])(void))
{
    struct FuncSeg *seg;
    int i, num;

    seg = (struct FuncSeg *)(((IPTR)FuncArray) - sizeof(struct FuncSeg));
    num = (seg->size - sizeof(struct FuncSeg)) / sizeof(APTR);

    for (i=0; (i < num) && FuncArray[i]; i++)
    {
        if (FuncArray[i] != ((const void (* const)(void))-1))
            (*FuncArray[i])();
    }
}

static int comp_ctdt(struct CTDT *a, struct CTDT *b)
{
    if (a->priority == b->priority)
        return (0);
    if ((unsigned long)a->priority < (unsigned long) b->priority)
        return (-1);

    return (1);
}

static struct CTDT *sort_ctdt(struct CTDT **last)
{
    struct FuncSeg *seg;
    struct CTDT *last_ctdt;

    seg = (struct FuncSeg *)(((IPTR)__ctdtlist) - sizeof(struct FuncSeg));
    last_ctdt = (struct CTDT *)(((IPTR)seg) + seg->size);

    qsort((struct CTDT *)__ctdtlist, (IPTR)(last_ctdt - __ctdtlist), sizeof(*__ctdtlist), (int (*)(const void *, const void *))comp_ctdt);

    *last = last_ctdt;

    return ((struct CTDT *) __ctdtlist);
}
//-
//+ InitLibnix
static BOOL InitLibnix(void)
{
    libnix_call = TRUE;
    
    /* Sort SAS/C de/constructor list */
    ctdt = sort_ctdt(&last_ctdt);

    /* Call SAS/C constructors */
    libnix_call = TRUE;
    while (ctdt < last_ctdt)
    {
        if (ctdt->priority >= 0)
        {
            if (setjmp(libnix_jmpbuf)) return FALSE;
            if (ctdt->fp() != 0) return FALSE;
        }

        ctdt++;
    }
    libnix_call = FALSE;

    /* Call ANSI constructors */
    CallFuncArray(__ctrslist);

    libnix_call = FALSE;

    return TRUE;
}
//-
//+ UnInitLibnix
static void UnInitLibnix(void)
{
    /* Call ANSI destructors */
    if (ctdt == last_ctdt) CallFuncArray(__dtrslist);

    /* Call SAS/C destructors */
    ctdt = (struct CTDT *)__ctdtlist;
    while (ctdt < last_ctdt)
    {
        if (ctdt->priority < 0)
        {
            if(ctdt->fp != (int (*)(void)) -1)
                ctdt->fp();
        }

        ctdt++;
    }
}
//-

//+ null_malloc
static void *null_malloc(size_t s) { return NULL; }
//-
//+ null_free
static void null_free(void *p) { }
//-


/*
** Public Functions
*/

/* Functions that could be replaced by the user */

//+ PyMorphOS_InitModule
BOOL PyMorphOS_InitModule(void) __attribute__ (( weak ));
BOOL PyMorphOS_InitModule(void) { return TRUE; }
//-
//+ PyMorphOS_TermModule
void PyMorphOS_TermModule(void) __attribute__ (( weak ));
void PyMorphOS_TermModule(void) {}
//-

/* Functions called by the dynload system */

//+ __PyMorphOS_InitModule
BOOL __PyMorphOS_InitModule(pPythonLibrary_t _PythonBase)
{
    BOOL res;
    BPTR lock;
    pPyMorphOS_ThreadData_t td;

    /* Library bases */
    PythonBase = _PythonBase;
    DPRINT("Hi! Base = %p\n", PythonBase);

    SysBase = PythonBase->MySysBase;
    DOSBase = PythonBase->MyDOSBase;

    Wrapper_malloc = PythonBase->Exported.ex_malloc;
    Wrapper_free = PythonBase->Exported.ex_free;

    DynLoadBase = OpenLibrary("dynload.library", 50);

    libnix_call = FALSE;
    libnix_LocaleBase = PythonBase->Exported.ex_LocaleBase;

    td = GET_THREAD_DATA_PTR();
    TimerBase = GET_THREAD_DATA(td, TimerBase);

    /* Constructors/Destructors calls */
    lock = CurrentDir(0); // some libraries changes the current dir when it opens :-( (like powersdl)
    res = InitLibnix(); // done here to not Lock() the module file inside dlopen()
    if (lock)
        CurrentDir(lock);

    if (res) {
        /* STDIO initialisations */
        old_stdio[0] = stdin;
        old_stdio[1] = stdout;
        old_stdio[2] = stderr;

        stdin = PythonBase->Exported.ex_stdin;
        stdout = PythonBase->Exported.ex_stdout;
        stderr = PythonBase->Exported.ex_stderr;

        /* Globals Python vars access */
        APTR gvars = PyMorphOS_GetGVars();
        CopyMem(gvars, &__pym_GVars, sizeof(__pym_GVars));

        /* User initialisation */
        res = PyMorphOS_InitModule();
    }

    return res;
}
//-
//+ __PyMorphOS_TermModule
void __PyMorphOS_TermModule(void)
{
    DPRINT("Closing all module's resources...\n");

    /* User termination */
    PyMorphOS_TermModule();

    stdin  = old_stdio[0];
    stdout = old_stdio[1];
    stderr = old_stdio[2];

    UnInitLibnix();

    if (NULL != DynLoadBase) CloseLibrary(DynLoadBase);

    DPRINT("Bye!\n");
}
//-

//+ exit
__dead void exit( int err ) __attribute__((noreturn));
void exit( int err )
{
    __dead void (*main_exit)(int) __attribute__((noreturn)) = PythonBase->Exported.ex_exit;

    DPRINT("exit code @ %p, libnix_call = %s\n",
           main_exit, libnix_call?"TRUE":"FALSE");

    /* As can be called during the libnix init process (InitLibnix) */
    if (libnix_call) longjmp(libnix_jmpbuf, 1);
    main_exit(err);
}
//-
//+ _exit
__dead void _exit( int err ) __attribute__((noreturn));
void _exit( int err )
{
    __dead void (*main__exit)(int) __attribute__((noreturn)) = PythonBase->Exported.ex__exit;
    
    DPRINT("_exit code @ %p, libnix_call = %s\n",
           main__exit, libnix_call?"TRUE":"FALSE");
    if (libnix_call) longjmp(libnix_jmpbuf, 1);
    main__exit(err);
}
//-
//+ malloc
void *malloc(size_t s)
{ return Wrapper_malloc(s); }
//-
//+ free
void free(void *p)
{ Wrapper_free(p); }
//-

/*
** Public Functions
*/

#include "bincomp.c"

/* De/constructor section-placeholders (MUST be last in the source (don't compile this with -O3)!) */
__asm("\n.section \".ctdt\",\"a\",@progbits\n__ctdtlist:\n.long -1,-1\n");
__asm("\n.section \".ctors\",\"a\",@progbits\n__ctrslist:\n.long -1\n");
__asm("\n.section \".dtors\",\"a\",@progbits\n__dtrslist:\n.long -1\n");
