/*******************************************************************************
 *** $Id:$
 ***
 *** Constructor/Destructor to open automaticaly the python.library.
 ***
 *******************************************************************************
 */

/*
** Project Includes
*/

#include "Python.h"


/*
** System Includes
*/

#include <exec/types.h>
#include <intuition/intuition.h>

#include <constructor.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "libraries/python2_gvars.h"
#include "libheader.h"


/*
** Public Prototypes
*/

void _INIT_4_PythonBase(void) __attribute__((alias("__CSTP_init_PythonBase")));
void _EXIT_4_PythonBase(void) __attribute__((alias("__DSTP_cleanup_PythonBase")));


/*
** Public Variables
*/

pPythonLibrary_t PythonBase /*__attribute__((weak))*/ = NULL;
struct PyMorphOS_GVar_STRUCT __pym_GVars;

extern unsigned long __stack;


/*
** Private Variables
*/

static CONST_STRPTR libname = LIBRARY_SHORTNAME;


/*
** Private Functions
*/

//+ openpythonerror()
static void openpythonerror(ULONG version, const char *name)
{
    struct IntuitionBase *IntuitionBase;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
    if (IntuitionBase)
    {
        static struct EasyStruct panic =
        {
            sizeof(struct EasyStruct),
            0,
            "Python startup message",
            0,
            "Abort"
        };
        ULONG args[2];

        panic.es_TextFormat = "You need version %.10ld of %.32s";
        args[0] = version;
        args[1] = (ULONG)name;
        EasyRequestArgs(NULL, &panic, NULL, args);

        CloseLibrary((struct Library *) IntuitionBase);
    }
}
//-
//+ init_PythonBase()
/* Priority shall be up to the stdio constructor (currently libnix stdio prio is 118)! */
CONSTRUCTOR_P(init_PythonBase, 150)
{
    PythonBase = (pPythonLibrary_t) OpenLibrary((STRPTR)libname, VERSION);

    if (NULL != PythonBase) {
        /* Globals sharing.
         * These symbols are exported by client applications.
         * So, the python library code and python modules use
         * same values.
         * Note that modules don't call this rountine.
         */
        struct TagItem tags[] = {
            {PYMOSATTR_GVARS_STORAGE, (ULONG)&__pym_GVars},
            {PYMOSATTR_EXIT_FUNC,     (ULONG)exit},
            {PYMOSATTR__EXIT_FUNC,    (ULONG)_exit},
            {PYMOSATTR_STDIN,         (ULONG)stdin},
            {PYMOSATTR_STDOUT,        (ULONG)stdout},
            {PYMOSATTR_STDERR,        (ULONG)stderr},
            {PYMOSATTR_MALLOC_FUNC,   (ULONG)malloc},
            {PYMOSATTR_FREE_FUNC,     (ULONG)free},
            {TAG_DONE, 0}
        };

        if (!PyMorphOS_SetConfigA(0, tags))
            return FALSE;

        CloseLibrary((struct Library *)PythonBase);
        PythonBase = NULL;
    } else
        openpythonerror(VERSION, libname);

    return TRUE;
}
//-
//+ cleanup_PythonBase()
/* shall be called before the libnix destructors! */
DESTRUCTOR_P(cleanup_PythonBase, 150)
{
    if (NULL != PythonBase) {
        PyMorphOS_Term();
        CloseLibrary((struct Library *)PythonBase);
        PythonBase = NULL;
    }
}
//-

/*
** Public Functions
*/

#include "bincomp.c"
