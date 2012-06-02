#include "morphos.h"

#undef dprintf
//#define DPRINT dprintf

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/debug_protos.h>
#include <dos/dostags.h>

#include <setjmp.h>

#define THREAD_STACKSIZE 1024*64

//+ __restore_r13()
/* This function must preserve all registers except r13 */
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

static __saveds void mos_bootstrap(pPythonLibrary_t base, void (*func)(void *), void *arg, jmp_buf *jmpbuf)
{
    pPyMorphOS_ThreadData_t td;

    DPRINT("<task %p>: base=%p, PythonBase=%p, jmpbuf=%p\n", FindTask(NULL), base, PythonBase, jmpbuf);

    td = (pPyMorphOS_ThreadData_t) PyMorphOS_InitThread();
    if (NULL != td) {
        SET_THREAD_DATA(td, JmpBuf, jmpbuf);

        if (!setjmp(*jmpbuf)) {
            DPRINT("<task %p>: run func @ %p with arg = %p...\n", FindTask(NULL), func, arg);
            func(arg);
            DPRINT("<task %p>: exit\n");
        }

        PyMorphOS_TermThread();
    }

    FreeMem(jmpbuf, sizeof(jmp_buf));
    DPRINT("<task %p>: Bye!\n", FindTask(NULL));
}

/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
    /* nothing */
}

/*
 * Thread support.
 */
long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    struct Process *proc;
    jmp_buf *jmpbuf;
    long pid;

    DPRINT("PyThread_start_new_thread called\n");
    if (!initialized) PyThread_init_thread();

    jmpbuf = AllocMem(sizeof(jmp_buf), MEMF_PUBLIC);
    if (!jmpbuf) return -1;

    /* On MorphOS, 2 threads = 2 processes.
     * As the Python code executed is r13 base relative, each thread shall obtain the same r13
     * value to point on the same data space => threads share the same data space.
     */

    proc = CreateNewProcTags(
        NP_CodeType,        CODETYPE_PPC,
        NP_Entry,           (ULONG) mos_bootstrap,
        NP_PPCStackSize,    THREAD_STACKSIZE,
        NP_Priority,        0,
        NP_Name,            (ULONG) "Python thread",
        //NP_Cli,             TRUE,
        NP_PPC_Arg1,        (ULONG) PythonBase,
        NP_PPC_Arg2,        (ULONG) func,
        NP_PPC_Arg3,        (ULONG) arg,
        NP_PPC_Arg4,        (ULONG) jmpbuf,
        TAG_DONE);

    DPRINT("CreateNewProcTags() = %p (%u)\n", proc, proc->pr_Task.tc_ETask->UniqueID);

    if (!proc) {
        DPRINT("can't create a new process\n");
        FreeMem(jmpbuf, sizeof(jmp_buf));
        return -1;
    }

    if (!NewGetTaskAttrsA((struct Task *)proc, &pid, sizeof(pid), TASKINFOTYPE_PID, 0))
        pid = (long)proc;

    return pid;
}

long
PyThread_get_thread_ident(void)
{
    if (!initialized) PyThread_init_thread();
    return (long) FindTask(NULL)->tc_ETask->UniqueID;
}

void
PyThread_exit_thread(void)
{
    jmp_buf *jmpbuf = GET_THREAD_DATA(GET_THREAD_DATA_PTR(), JmpBuf);

    DPRINT("PyThread_exit_thread called\n");
    if (!initialized) {
        exit(0);
    }

    longjmp(*jmpbuf, 1);
}

typedef struct
{
    struct MinNode Node;
    struct SignalSemaphore Sem;
    struct MinList WaitList;
    BOOL Locked;
} Lock_t, *pLock_t;

typedef struct
{
    struct MinNode Node;
    struct Task *Task;
    ULONG SigMask;
} Waiter_t, *pWaiter_t;

/*
 * Lock support.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
    pLock_t lock;

    DPRINT("PyThread_allocate_lock called\n");
    if (!initialized) PyThread_init_thread();

    lock = malloc(sizeof(Lock_t));
    if (!lock) return NULL;

    InitSemaphore(&lock->Sem);
    NEWLIST(&lock->WaitList);
    lock->Locked = FALSE;

    DPRINT("PyThread_allocate_lock() -> %p\n", lock);
    return (PyThread_type_lock) lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
    pLock_t _lock = (pLock_t)lock;
    pWaiter_t waiter;

    DPRINT("PyThread_free_lock(%p) called\n", lock);

    ObtainSemaphore(&_lock->Sem);

    ForeachNode(&_lock->WaitList, waiter) {
        Signal(waiter->Task, waiter->SigMask);
    }

    ReleaseSemaphore(&_lock->Sem);

    free(_lock);
}

int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    pLock_t _lock = (pLock_t)lock;
    int success = 1;

    DPRINT("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag);

    ObtainSemaphore(&_lock->Sem);

    if (_lock->Locked) {
        if (waitflag) {
            Waiter_t waiter;
            LONG signal;
            ULONG sigs;

            signal = AllocSignal(-1);
            if (-1 == signal) {
                signal = SIGBREAKB_CTRL_E;
                SetSignal(SIGBREAKF_CTRL_E, 0);
            }

            waiter.Task = FindTask(NULL);
            waiter.SigMask = 1 << signal;

            ADDTAIL(&_lock->WaitList, &waiter);

            ReleaseSemaphore(&_lock->Sem);

            sigs = Wait(waiter.SigMask | SIGBREAKF_CTRL_C);

            ObtainSemaphore(&_lock->Sem);

            REMOVE(&waiter);

            if (signal != SIGBREAKB_CTRL_E) FreeSignal(signal);
            if (sigs & SIGBREAKF_CTRL_C) success = 0;
        }
        else
            success = 0;
    }
    else
        _lock->Locked = TRUE;

    ReleaseSemaphore(&_lock->Sem);

    DPRINT("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success);
    return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    pLock_t _lock = (pLock_t)lock;
    pWaiter_t waiter;

    DPRINT("PyThread_release_lock(%p) called\n", lock);

    ObtainSemaphore(&_lock->Sem);

    if (_lock->Locked) {
        waiter = (pWaiter_t) GetHead(&_lock->WaitList);
        if (waiter)
            Signal(waiter->Task, waiter->SigMask);
        else
            _lock->Locked = FALSE;
    }

    ReleaseSemaphore(&_lock->Sem);
}

/* set the thread stack size.
 * Return 0 if size is valid, -1 if size is invalid,
 * -2 if setting stack size is not supported.
 */
static int
_morphos_pthread_set_stacksize(size_t size)
{
	/* TODO */
	return -2;
}

#define THREAD_SET_STACKSIZE(x) _morphos_pthread_set_stacksize(x)
