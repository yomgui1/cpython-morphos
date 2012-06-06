/*******************************************************************************
 *** $Id:$
 ***
 *** Morphos specifics functions to initialise a standalone Python executable.
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#include "Python.h"
#include "morphos.h"
#include "mosdebug.h"
#include "libraries/python27_gvars.h"


/*
** System Includes
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/icon.h>
#include <proto/timer.h>

#include <workbench/startup.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <clib/debug_protos.h>

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/file.h>
#include <locale.h>


/*
** Private Macros and Definitions
*/

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "Python"
#endif

#ifndef PROGRAM_VER
#define PROGRAM_VER "X.X"
#endif

#ifndef COMPILE_DATE
#define COMPILE_DATE __DATE__
#endif

#ifndef COMPILE_TIME
#define COMPILE_TIME __TIME__
#endif

#define DEFAULT_CONSOLE "CON:100/100/640/480/" PROGRAM_NAME " - Console/CLOSE/WAIT"
#define ICON_LIBRARY_VERSION 39

/* this define fix the maximal number of command line argument.
** Just a security in case of a wrong argument pointer.
*/

#define MAX_ARGC 2048

/*===== FILE PRIVATE SECTION =================================================*/

/*
** Private Types and Structures
*/

typedef struct _MyLibNode
{
    struct MinNode      mln_Node;
    struct Library *    mln_Lib;
} MyLibNode_t, *pMyLibNode_t;


typedef struct _TermFuncNode
{
    struct MinNode      tfn_Node;
    void                (*tfn_Func)(void);
#ifndef NDEBUG
    STRPTR              tfn_Name;
#endif
} TermFuncNode_t, *pTermFuncNode_t;


/*
** Private Prototypes
*/

static ULONG readArgs( int *, char *** );


/*
** Private Variables
*/

static void* null_malloc(size_t s);
static void null_free(void *p);

static __dead void (*Wrapper_exit)(int) __attribute__((noreturn)) = NULL;
static __dead void (*Wrapper__exit)(int) __attribute__((noreturn)) = NULL;
static void* (*Wrapper_malloc)(size_t s) = null_malloc;
static void (*Wrapper_free)(void *p) = null_free;
static FILE *old_stdio[3] = {0};
static BOOL gInitDone = FALSE;

/* De/constructor stuff */
extern const void (* const __ctrslist[])(void);
extern const void (* const __dtrslist[])(void);

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

extern const struct CTDT __ctdtlist[];
static struct CTDT *sort_ctdt(struct CTDT **last);
static struct CTDT *ctdt, *last_ctdt;

/* Notes that all these globals will be shared with threads using python code.
 * But not neccessary the threads code itself ;-)
 */

struct ExecBase *   SysBase = NULL;
struct DosLibrary * DOSBase = NULL;
struct Library *    UtilityBase = NULL;
struct Library *    UserGroupBase = NULL;
struct Library *    DynLoadBase = NULL;
pPythonLibrary_t    PythonBase = NULL;

struct Task *gMainThread = NULL;
BOOL gStartedFromWB = FALSE;
BPTR gOldProgDirLock = 0;
STRPTR gProgDir = NULL;
STRPTR gProgName = NULL;
STRPTR gConsoleDesc = NULL;
STRPTR gToolTypesArray = NULL;
STRPTR *gArgv = NULL;
struct MinList gTermFuncList;
struct MinList gThreadDataList;
struct SignalSemaphore gGlobalSem;

/* Libnix globals */
APTR libnix_LocaleBase __attribute__ ((weak)) = NULL;
int ThisRequiresConstructorHandling = 1;


/*
** Private Functions
*/

//+ ANSI de/constructor handlers
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

static BOOL InitLibnix(void)
{
    /* Sort SAS/C de/constructor list */
    ctdt = sort_ctdt(&last_ctdt);

    /* Call SAS/C constructors */
    while (ctdt < last_ctdt)
    {
        if (ctdt->priority >= 0)
        {
            if(ctdt->fp() != 0)
            {
                return FALSE;
            }
        }

        ctdt++;
    }

    /* Call ANSI constructors */
    CallFuncArray(__ctrslist);

    return TRUE;
}

static void UnInitLibnix(void)
{
    /* Call ANSI destructors */
    if (ctdt == last_ctdt)
        CallFuncArray(__dtrslist);

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
//+ readArgs
/* Note: when started from WB, the arguments are Tooltypes from program icon.
*/
static ULONG readArgs( int *pargc, char ***pargv )
{
    ULONG result = RETURN_FAIL;
    BPTR prgLock = 0;

    if (0 == *pargc) { /* Started from WB */
        struct Library      *IconBase = NULL;
        struct WBStartup    *wb_msg;
        struct WBArg        *wb_arg;
        struct DiskObject   *dskObj = NULL;
        BPTR                oldDirLock = 0;
        APTR                *ttTable = NULL;
        ULONG               i, argvCnt = 0;

        gStartedFromWB = TRUE;

        wb_msg = (struct WBStartup *)*pargv;
        wb_arg = wb_msg->sm_ArgList;

        DPRINT("Started from WB (argc = %lu)\n", wb_msg->sm_NumArgs);

        IconBase = OpenLibrary("icon.library", ICON_LIBRARY_VERSION);
        if (IconBase != NULL) {
            /* Read program name and ToolTypes from icons.
            ** Check for extra icon too (multi-selection).
            */
            for (i=0; i < wb_msg->sm_NumArgs; i++) {
                BPTR lock;
                STRPTR name;

                lock = wb_arg[i].wa_Lock;
                name = wb_arg[i].wa_Name;

                if (0 != lock) {
                    switch (i) {
                        case 0: // Program Icon Path + Name
                            prgLock = lock;
                            oldDirLock = CurrentDir(lock);

                            // set program name
                            gProgName = name;

                            // parse ToolTypes
                            {
                                ULONG ttCnt;
                                ULONG toolTypesArraySize;

                                dskObj = GetDiskObject(name);
                                if (NULL != dskObj) {
                                    // count the number of ToolTypes
                                    ttCnt = 0;
                                    while (dskObj->do_ToolTypes[ttCnt] != NULL)
                                        ttCnt++;

                                    DPRINT("%lu ToolTypes found\n", ttCnt);

                                    // make a copy of ToolTypes array but without specials ToolTypes
                                    ttTable = malloc(sizeof(APTR) * (ttCnt + 1)); // +1 for the trailing NULL
                                    if (NULL != ttTable) {
                                        STRPTR * pStr = dskObj->do_ToolTypes;

                                        i = 0;
                                        toolTypesArraySize = 0;
                                        for (; *pStr != NULL; pStr++) {
                                            ULONG l = strlen(*pStr);

                                            // check if ToolType is enable
                                            if (((*pStr)[0] != '(') || ((*pStr)[l-1] != ')')) {
                                                /* check for specials ToolTypes
                                                */

                                                if (!strncmp(*pStr, "WINDOW=", 7)) {
                                                    STRPTR p;

                                                    /* WINDOW=ConDevice:X/Y/W/H/Title/Options */

                                                    /* freeing any old console string */
                                                    if (NULL != gConsoleDesc) {
                                                        free(gConsoleDesc);
                                                        gConsoleDesc = NULL;
                                                    }

                                                    p = strchr(*pStr, '=') + 1;
                                                    if (NULL != p) {
                                                        gConsoleDesc = malloc(strlen(p) + 1);
                                                        if (NULL != gConsoleDesc) {
                                                            strcpy(gConsoleDesc, p);
                                                            DPRINT("Console: '%s'\n", gConsoleDesc);
                                                        }
                                                        else
                                                            DMSG_NOMEM();
                                                    }
                                                    else
                                                        DPRINT("malformed WINDOW ToolTypes\n");
                                                } else {
                                                    // add this tooltype for argv
                                                    ttTable[i++] = *pStr;
                                                    toolTypesArraySize += l;
                                                }
                                            }
                                        }

                                        ttTable[i] = NULL;
                                        argvCnt += i;
                                        DPRINT("remaining %lu ToolTypes for argv (take %lu bytes)\n", i, toolTypesArraySize);

                                        if (toolTypesArraySize > 0)
                                            gToolTypesArray = malloc(toolTypesArraySize);
                                    } else
                                        DMSG_NOMEM();
                                } else
                                    DPRINT_ERROR("no disk object found for '%s'\n", name);
                            }
                            break;

                        case 1: // Project Icon Path
                            /* TODO */
                            break;

                        default: // Extended Icons Path
                            break;
                    }
                }
            }

            /* allocate our argv array
            ** This array contains for the first index the program name (fullpath),
            ** then Tooltypes strings.
            ** ANSI-C argv must be terminated with a NULL pointer
            ** and argv[0] is always the program name => argvCnt + 2.
            */
            *pargv = malloc((argvCnt + 2) * sizeof(STRPTR));
            gArgv = (STRPTR *) *pargv; /* deallocated at exit */
            if (NULL !=  *pargv) {
                STRPTR *argv = (STRPTR *) *pargv;
                ULONG argc = 1;

                // put in argv[0] the program name pointer
                argv[0] = gProgName;

                /* if there are ToolTypes from WBStartup parsing, copy it into our argv array
                ** but only if it's not an disabled ToolTypes like this "(MyDisabledToolType=<blabla>)"
                */
                if (NULL != gToolTypesArray) {
                    STRPTR str = gToolTypesArray;

                    // Add ToolTypes in our argv array
                    for (i = 0; NULL != ttTable[i]; i++, argc++) {
                        strcpy(str, ttTable[i]);
                        argv[argc] = str;

                        // next string position
                        str += strlen(str);
                        (str++)[0] = '\0';

                    }

                    argvCnt -= i;

                    free(ttTable);
                }

                // ANSI-C terminator
                argv[argc] = NULL;

                *pargc = argc;
            } else
                DMSG_NOMEM();

            // freeing tempory ressources
            if (NULL != dskObj)
                FreeDiskObject(dskObj);

            if (0 != oldDirLock)
                CurrentDir(oldDirLock);

            CloseLibrary(IconBase);
        } else
            DPRINT_ERROR("can't open icon.libray >= V%u\n", ICON_LIBRARY_VERSION);
    } else { /* Started from Cli */
        char * arg = *pargv[0];

        DPRINT("Started from CLI (argc = %lu)\n", *pargc);
        DPRINT("Argv[0] = '%s'\n", arg);

        gStartedFromWB = FALSE;

        gProgDir = malloc(strlen(arg)+1);
        DPRINT("gProgDir @ %p\n", gProgDir);
        if (NULL != gProgDir) {
            strcpy(gProgDir, arg);
            *(PathPart(gProgDir)) = '\0';
            gProgName = FilePart(arg);
            DPRINT("gProgName: '%s'\n", gProgName);

            /* BUG FIX: sometimes argc is not correct...
            ** The NULL ANSI-C argument is taken in account (??)
            ** But sometimes there is an empty string (!?)
            ** correction => argc -= 1
            **
            ** UPDATE 070107: Remove the '\0' case... or empty but
            ** given between double quotes string are not taken in account!
            */
            arg = (*pargv)[*pargc - 1];
            if (NULL == arg) (*pargc)--;

            // try to obtain a lock on program path
            prgLock = GetProgramDir();
        } else
            DMSG_NOMEM();
    }

    /* get program path name from a lock */
    if (0 != prgLock) {
        BOOL res;
        APTR ptr;

        // try to obtain a name from the lock
        ptr = malloc(MAXPATHLEN + 1);
        gProgDir = ptr;
        if (ptr != NULL) {
            res = NameFromLock(prgLock, ptr, MAXPATHLEN + 1);
            if (res) {
                /* we have everythings that we need... ;-) */
                result = RETURN_OK;
            } else
                DPRINT_ERROR("can't get name from lock\n");
        } else
            DMSG_NOMEM();
    } else
        DPRINT_ERROR("no lock on a program path\n");

    #ifndef NDEBUG
    {
        ULONG i;
        for (i=0; i < *pargc; i++) DPRINT("argv[%d]: '%s'\n", i, (*pargv)[i]);
    }
    #endif

    return result;
}
//-
//+ null_malloc
static void *null_malloc(size_t s) { kprintf("%p called (return NULL)\n", __FUNCTION__); return NULL; }
//-
//+ null_free
static void null_free(void *p) { kprintf("%p called\n", __FUNCTION__); }
//-

/*===== LIBRARY EXPOSED SECTION ==============================================*/

//+ PyMorphOS_SetConfigA
int PyMorphOS_SetConfigA(int dummy, struct TagItem *tags)
{
    struct TagItem *tag;
    FILE *prog_stdin = NULL;
    FILE *prog_stdout = NULL;
    FILE *prog_stderr = NULL;
    APTR gvars_storage = NULL;

    DPRINTRAW("\n+********************** MORPHOS PYTHON INIT ****************************\n\n");
    DPRINT("PythonBase = %p\n", PythonBase);

    gInitDone = FALSE;

    if (NULL == tags) {
        DPRINT_ERROR("tags can't be NULL!\n");
        return RETURN_FAIL;
    }

    /* Needed for dlopen and co */
    DynLoadBase = OpenLibrary("dynload.library", 50);
    if (NULL != DynLoadBase) {
        /* Needed for morphosmodule */
        UserGroupBase = OpenLibrary("usergroup.library", 50);
        if (NULL != UserGroupBase) {
            /* Read tags */
            while (NULL != (tag = NextTagItem(&tags))) {
                switch (tag->ti_Tag) {
                    case PYMOSATTR_GVARS_STORAGE: gvars_storage  = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_EXIT_FUNC:     Wrapper_exit   = (APTR)tag->ti_Data; break;
                    case PYMOSATTR__EXIT_FUNC:    Wrapper__exit  = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_STDIN:         prog_stdin     = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_STDOUT:        prog_stdout    = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_STDERR:        prog_stderr    = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_MALLOC_FUNC:   Wrapper_malloc = (APTR)tag->ti_Data; break;
                    case PYMOSATTR_FREE_FUNC:     Wrapper_free   = (APTR)tag->ti_Data; break;
                    default:;
                }
            }

            DPRINT("program exit/_exit func: [%p, %p]\n", Wrapper_exit, Wrapper__exit);
            DPRINT("program malloc/free func: [%p, %p]\n", Wrapper_malloc, Wrapper_free);
            DPRINT("program gvars storage: %p\n", gvars_storage);
            DPRINT("program stdio streams: [%p, %p, %p]\n", prog_stdin, prog_stdout, prog_stderr);

            InitSemaphore(&gGlobalSem);

            /* Check mandatory values */
            if (prog_stdin && prog_stdout && prog_stderr && gvars_storage && Wrapper_exit && Wrapper__exit
                && Wrapper_malloc && Wrapper_free) {
                pPyMorphOS_ThreadData_t td;

                gMainThread = FindTask(NULL);

                NEWLIST(&gTermFuncList);
                NEWLIST(&gThreadDataList);

                PythonBase->Exported.ex_stdin = prog_stdin;
                PythonBase->Exported.ex_stdout = prog_stdout;
                PythonBase->Exported.ex_stderr = prog_stderr;
                PythonBase->Exported.ex_malloc = Wrapper_malloc;
                PythonBase->Exported.ex_free = Wrapper_free;
                PythonBase->Exported.ex_exit = Wrapper_exit;
                PythonBase->Exported.ex__exit = Wrapper__exit;

                /* The main thread is ... a thread! */
                td = PyMorphOS_InitThread();
                if (NULL != td) {
                    DPRINT("Init libnix stuffs...\n");
                    if (InitLibnix()) {
                        /* Replacing stdio streams (open during InitLibnix()) */
                        DPRINT("Replace stdio streams (old=[%p, %p, %p])...\n", stdin, stdout, stderr);

                        old_stdio[0] = stdin;
                        old_stdio[1] = stdout;
                        old_stdio[2] = stderr;
                        stdin = prog_stdin; stdout = prog_stdout; stderr = prog_stderr;
                        
                        PythonBase->Exported.ex_LocaleBase = libnix_LocaleBase;
                        _PyMorphOS_InitGVars(gvars_storage);
                        
                        setlocale(LC_ALL, "C");

                        gInitDone = TRUE;
                        return RETURN_OK;
                    } else
                        DPRINT_ERROR("InitLibnix() failed\n");

                    PyMorphOS_TermThread();
                } else
                    DPRINT_ERROR("can't initialize main thread data\n");
            } else
                DPRINT_ERROR("invalid required init values\n");

            CloseLibrary(UserGroupBase);
        } else
            DPRINT_ERROR("OpenLibrary(\"usergroup.library\", 50) failed\n");
    } else
        DPRINT_ERROR("OpenLibrary(\"dynload.library\", 50) failed\n");

    DPRINTRAW("\n-********************** MORPHOS PYTHON INIT ****************************\n");
    return RETURN_FAIL;
}
//-
//+ PyMorphOS_Term
void PyMorphOS_Term( void )
{
    APTR ptr;
    pTermFuncNode_t node;

    if (!gInitDone) return;

    /* Calling all termination user functions */
    ForeachNodeSafe(&gTermFuncList, node, ptr) {
        DPRINT("node=0x%p, calling term func ");
#ifndef NDEBUG
        if (node->tfn_Name) {
            DPRINTRAW("'%s' ", node->tfn_Name);
            free(node->tfn_Name);
        }
#endif
        DPRINTRAW("@ %p...\n", node->tfn_Func);

        node->tfn_Func();
        free(node);
    }

    if (gOldProgDirLock)
    {
        BPTR oldlock = SetProgramDir(gOldProgDirLock);

        DPRINT("Old PROGDIR: lock @ %p, unlock @ %p\n", gOldProgDirLock, oldlock);

        if (oldlock)
            UnLock(oldlock);

        gOldProgDirLock = 0;
    }


    /* Freeing allocated memory */
    DPRINT("freeing allocated globals...\n");
    if (NULL != gArgv) { free(gArgv); gArgv = NULL; }
    if (NULL != gToolTypesArray) { free(gToolTypesArray); gToolTypesArray = NULL; }
    if (NULL != gProgDir) { free(gProgDir); gProgDir = NULL; }

    /* Restore stdio streams */
    stdin = old_stdio[0];
    stdout = old_stdio[1];
    stderr = old_stdio[2];

    /* Deallocation of last resources */
    DPRINT("Ending main thread...\n");
    PyMorphOS_TermThread();
    if (NULL != UserGroupBase) { CloseLibrary(UserGroupBase); UserGroupBase = NULL; }
    if (NULL != DynLoadBase) { CloseLibrary(DynLoadBase); DynLoadBase = NULL; }

    UnInitLibnix();

    DPRINTRAW("\n-********************** MORPHOS PYTHON TERM ****************************\n\n");
}
//-
//+ PyMorphOS_HandleArgv
int PyMorphOS_HandleArgv(int *pargc, char ***pargv)
{
    int result;

    /* check arguments */
    if (!(pargc && pargv) || (*pargc > MAX_ARGC)) {
        DPRINT("wrong arguments: pargc=0x%08p (%lu), pargv=0x%08p\n", pargc, pargc?*pargc:0, pargv);
        return RETURN_FAIL;
    }

    /* Read WB/Cli arguments */
    result = readArgs(pargc, pargv);
    if (RETURN_OK == result) {
        DPRINT("Program dir is '%s'\n", gProgDir);
        DPRINT("Program name is '%s'\n", gProgName);
    } else
        DPRINT_ERROR("error during read of program arguments\n");

    return result;
}
//-
//+ PyMorphOS_GetGVars
APTR PyMorphOS_GetGVars( void )
{
    DPRINT("PythonGVars @ %p\n", PythonBase->PythonGVars);
    return PythonBase->PythonGVars;
}
//-
//+ PyMorphOS_GetFullPath
LONG PyMorphOS_GetFullPath( const char *path, char *buffer, ULONG size )
{
    BPTR lock = 0;
    LONG result = FALSE;

    if (size > 0) {
        lock = Lock(path, SHARED_LOCK);
        if (0 != lock) {
            result = NameFromLock(lock, buffer, size);
            UnLock(lock);
        }
    }

    return result;
}
//-
//+ PyMorphOS_AddTermFunc
LONG PyMorphOS_AddTermFunc( void (*func)(void), CONST_STRPTR name )
{
    pTermFuncNode_t node;

    if (!func) return -1;

    node = malloc(sizeof(TermFuncNode_t));
    if (!node) return -1;

    node->tfn_Func = func;

    #ifndef NDEBUG
    {
        ULONG size = 0;
        if (name) {
            size = strlen(name);
            node->tfn_Name = malloc(size + 1);
        } else
            node->tfn_Name = NULL;

        if (node->tfn_Name) {
            CopyMem((char *)name, node->tfn_Name, size);
            node->tfn_Name[size] = '\0';
        }
    }
    #endif

    ADDHEAD(&gTermFuncList, node);
    DPRINT("New func %p added at node %p\n", func, node);

    return 0;
}
//-
//+ PyMorphOS_InitThread
/* This function should be called by the thread itself to support Python calls.
** Data structure should be allocated with the same memory pool for each thread.
*/
APTR PyMorphOS_InitThread(void)
{
    pPyMorphOS_ThreadData_t td;
    struct MsgPort *popen_port, *timer_port;

    td = AllocMem(sizeof(PyMorphOS_ThreadData_t), MEMF_PUBLIC|MEMF_CLEAR|MEMF_SEM_PROTECTED);
    if (NULL != td) {
        ObtainSemaphore(&gGlobalSem);
        ADDTAIL(&gThreadDataList, td);
        ReleaseSemaphore(&gGlobalSem);

        SET_THREAD_DATA_PTR(td);

        // Create a message port for popen
        popen_port = CreateMsgPort();
        DPRINT("POpenPort: %p\n", popen_port);
        if (NULL != popen_port)
            SET_THREAD_DATA(td, POpenPort, popen_port);
        else
            DPRINT_ERROR("can't create a message port for popen\n");

        // Create a message port for timer things
        timer_port = CreateMsgPort();
        DPRINT("TimerPort: %p\n", timer_port);
        if (NULL != timer_port)
            SET_THREAD_DATA(td, TimerPort, timer_port);
        else
            DPRINT_ERROR("can't create a message port for timer\n");

        if ((NULL != popen_port) && (NULL != timer_port)) {
            struct timerequest *tr;

            tr = (struct timerequest *)CreateIORequest(timer_port, sizeof(*tr));
            if (NULL != tr) {
                BYTE error;

                SET_THREAD_DATA(td, TimeRequest, tr);
                error = OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ, (struct IORequest *)tr, 0);
                if (!error) {
                    SET_THREAD_DATA(td, TimerBase, (struct Library *)tr->tr_node.io_Device);
                    return td;
                } else
                    DPRINT_ERROR("can't open the timer.device on unit UNIT_VBLANK\n");
            } else
                DPRINT_ERROR("CreateIORequest() failed\n");
        }

        /* Destroying anything */
        PyMorphOS_TermThread();
    }

    return NULL;
}
//-
//+ PyMorphOS_TermThread
void PyMorphOS_TermThread(void)
{
    pPyMorphOS_ThreadData_t td;
    struct MsgPort *port;
    struct timerequest *tr;       
    struct Message *msg;

    td = GET_THREAD_DATA_PTR();        
    if (NULL != td) {
        ObtainSemaphore(&gGlobalSem);
        REMOVE(td);
        ReleaseSemaphore(&gGlobalSem);

        tr = GET_THREAD_DATA(td, TimeRequest);
        if (NULL != tr) {
            APTR base = GET_THREAD_DATA(td, TimerBase);
            if (NULL != base) {
                CloseDevice((struct IORequest *)tr);
                SET_THREAD_DATA(td, TimerBase, NULL);
            }

            DeleteIORequest(tr);
            SET_THREAD_DATA(td, TimeRequest, NULL);
        }

        port = GET_THREAD_DATA(td, TimerPort);
        DPRINT("TimerPort: %p\n", port);
        if (NULL != port) {
            DeleteMsgPort(port);
            SET_THREAD_DATA(td, TimerPort, NULL);
        }

        port = GET_THREAD_DATA(td, POpenPort);
        DPRINT("POpenPort: %p\n", port);
        if (NULL != port) {
            while (NULL != (msg = GetMsg(port)));
            DeleteMsgPort(port);
            SET_THREAD_DATA(td, POpenPort, NULL);
        }

        SET_THREAD_DATA_PTR(NULL);
        FreeMem(td, sizeof(PyMorphOS_ThreadData_t));
    }
}
//-

//+ _PyMorphOS_SetProgDir
void _PyMorphOS_SetProgDir(BPTR lock)
{
    BPTR oldlock;

    DPRINT("Use PROGDIR: lock @ %p\n", lock);
    oldlock = SetProgramDir(lock);
    if (gOldProgDirLock)
        UnLock(gOldProgDirLock);
    gOldProgDirLock = oldlock;
}
//-

/* Wrapped routines on application's ones */
void exit( int e ) { Wrapper_exit(e); }
void _exit( int e ) { Wrapper__exit(e); }
void *malloc(size_t s) { return Wrapper_malloc(s); }
void free(void *p) { Wrapper_free(p); }

/* Disable CTRL-C check in libc routines */
void __chkabort(void) {}


/* De/constructor section-placeholders (MUST be last in the source (don't compile this with -O3)!) */
__asm("\n.section \".ctdt\",\"a\",@progbits\n__ctdtlist:\n.long -1,-1\n");
__asm("\n.section \".ctors\",\"a\",@progbits\n__ctrslist:\n.long -1\n");
__asm("\n.section \".dtors\",\"a\",@progbits\n__dtrslist:\n.long -1\n");
