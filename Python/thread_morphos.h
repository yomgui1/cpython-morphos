#include "morphos.h"

#undef dprintf
//#define DPRINT dprintf

#include <clib/debug_protos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>

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

static
void do_PyThread_exit_thread(int no_cleanup)
{
    jmp_buf *jmpbuf = GET_THREAD_DATA(GET_THREAD_DATA_PTR(), JmpBuf);

    DPRINT("PyThread_exit_thread called\n");
    if (!initialized) {
        if (no_cleanup)
            _exit(0);
        else
            exit(0);
    }

    longjmp(*jmpbuf, 1);
}

void
PyThread_exit_thread(void)
{
    do_PyThread_exit_thread(0);
}

typedef struct
{
    struct MinNode Node;
    struct SignalSemaphore Sem;
    struct SignalSemaphore WaitListLock;
    struct MinList WaitList;
    BOOL Owned;
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

    if (!initialized) PyThread_init_thread();

    lock = malloc(sizeof(Lock_t));
    if (!lock) return NULL;

    InitSemaphore(&lock->Sem);
    InitSemaphore(&lock->WaitListLock);
    NEWLIST(&lock->WaitList);
    lock->Owned = FALSE;

    DPRINT("PyThread_allocate_lock(): new lock -> %p\n", lock);
    return (PyThread_type_lock) lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
    pLock_t _lock = (pLock_t)lock;
    pWaiter_t waiter;

    DPRINT("PyThread_free_lock(%p) called\n", lock);

    AttemptSemaphore(&_lock->Sem);

    ObtainSemaphore(&_lock->WaitListLock);
    ForeachNode(&_lock->WaitList, waiter)
        Signal(waiter->Task, waiter->SigMask);
    ReleaseSemaphore(&_lock->WaitListLock);
    
    free(_lock);
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock aLock,
                            PY_TIMEOUT_T microseconds,
                            int intr_flag)
{
    pLock_t _lock = (pLock_t)aLock;
    int success;

    DPRINT("PyThread_acquire_lock_timed(%p, %lld, %d) called\n", aLock, microseconds, intr_flag);

    if (AttemptSemaphore(&_lock->Sem)) {
        if (_lock->Owned) {
            success = PY_LOCK_FAILURE;
        } else {
            _lock->Owned = TRUE;
            success = PY_LOCK_ACQUIRED;
        }
        DPRINT("PyThread_acquire_lock_timed(%p): Attempt ok, r=%u\n", aLock, success);
    } else if (microseconds == 0) {
        DPRINT("PyThread_acquire_lock_timed(%p): Attempt failed\n", aLock);
        success = PY_LOCK_FAILURE;
    } else {
        Waiter_t me;
        pPyMorphOS_ThreadData_t td = GET_THREAD_DATA_PTR();
        struct timerequest * tr = GET_THREAD_DATA(td, TimeRequest);
        struct timerequest req = *tr;
        ULONG sigs, sigmask_timer;
        LONG sigbit;
        
        DPRINT("PyThread_acquire_lock_timed(%p) waiting...\n", aLock);
        sigbit = AllocSignal(-1);
        if (sigbit == -1)
            sigbit = SIGBREAKB_CTRL_E;

        me.Task = FindTask(NULL);
        me.SigMask = 1ul << sigbit;
        
        ObtainSemaphore(&_lock->WaitListLock);
        ADDTAIL(&_lock->WaitList, &me);
        ReleaseSemaphore(&_lock->WaitListLock);

        if (microseconds == -1) {
            req.tr_node.io_Command = TR_ADDREQUEST;
            req.tr_time.tv_secs = microseconds / 1000000;
            req.tr_time.tv_micro = microseconds % 1000000;
            SendIO((struct IORequest *) &req);
            sigmask_timer = 1ul << (GET_THREAD_DATA(td, TimerPort)->mp_SigBit);
        } else
            sigmask_timer = 0;

        sigs = Wait(me.SigMask | sigmask_timer);
        
        if (sigmask_timer && CheckIO((struct IORequest *) &req)) {
            AbortIO((struct IORequest *) &req);
            WaitIO((struct IORequest *) &req);
        }
                
        if (sigbit != SIGBREAKB_CTRL_E)
            FreeSignal(sigbit);
        
        if (sigs & sigmask_timer)
            success = PY_LOCK_FAILURE;
        else if (AttemptSemaphore(&_lock->Sem)) {
            _lock->Owned = TRUE;
            success = PY_LOCK_ACQUIRED;
        } else if (intr_flag)
            success = PY_LOCK_INTR;
        else
            success = PY_LOCK_FAILURE;
    }

    DPRINT("PyThread_acquire_lock_timed(%p) returns %u\n", aLock, success);
    return success;
}

int
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
    return PyThread_acquire_lock_timed(aLock, waitflag ? -1 : 0, 0);
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    pLock_t _lock = (pLock_t)lock;
    pWaiter_t waiter;

    DPRINT("PyThread_release_lock(%p) called\n", lock);

    ObtainSemaphore(&_lock->WaitListLock);
    ReleaseSemaphore(&_lock->Sem);
    _lock->Owned = FALSE;
    waiter = REMHEAD(&_lock->WaitList);
    if (waiter)
        Signal(waiter->Task, waiter->SigMask);
    ReleaseSemaphore(&_lock->WaitListLock);
}
