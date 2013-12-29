/*******************************************************************************
 *** $Id$
 ***
 ***
 *** Description:
 ***
 ***   Morphos specifics functions header.
 ***
 *** Notes:
 ***
 ***   Must not be included by any modules.
 ***
 *******************************************************************************
 */

#ifndef PYMORPHOS_H
#define PYMORPHOS_H

/*
** Project Includes
*/

#include "libheader.h"


/*
** System Includes
*/

#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/utility.h>

#include <sys/types.h>


/* Thread data access macros */
#define GET_THREAD_DATA_PTR() ((pPyMorphOS_ThreadData_t)(FindTask(NULL)->tc_UserData))
#define SET_THREAD_DATA_PTR(v) (FindTask(NULL)->tc_UserData = (APTR)(v))
#define GET_THREAD_DATA(td, x) (td->x)
#define SET_THREAD_DATA(td, x, v) (td->x = (v))

/* Misc */
#define SAFE_INFINITE_LOOP for (;0 == SetSignal(0,SIGBREAKF_CTRL_C);)


/*
** Public Types and Structures
*/

typedef void (*exitFct)(int);

/* Per thread data (globals are shared between threads created by the python.library) */
typedef struct PyMorphOS_ThreadData
{
    struct MinNode       SysNode;
    struct MsgPort *     POpenPort;           // msg port for popen emulation
    APTR                 JmpBuf;              // jmp_buf used to exit a thread
    struct MsgPort *     TimerPort;           // msg port for timer device operations
    struct timerequest * TimeRequest;
    struct Library *     TimerBase;
} PyMorphOS_ThreadData_t, *pPyMorphOS_ThreadData_t;

/*
** Globals
*/

extern struct Task *gMainThread;
extern BOOL gStartedFromWB;
extern STRPTR gProgDir;
extern STRPTR gProgName;
extern STRPTR gConsoleDesc;
extern STRPTR gToolTypesArray;
extern STRPTR *gArgv;
extern struct MinList gTermFuncList;
extern struct MinList gThreadDataList;
extern pPyMorphOS_ThreadData_t gThreadDataPtr;


/*
** Public Functions Prototypes
*/

#ifndef PYTHON_LIBTABLE
extern APTR PyMorphOS_InitThread(void);
extern void PyMorphOS_TermThread(void);
#endif /* PYTHON_LIBTABLE */

#endif /* PYMORPHOS_H */
