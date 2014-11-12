/* _subprocess.c : low level functions for subprocess module of MorphOS
 * By Guillaume Roguez <yomgui1@gmail.com>
 * Inspired from Windows _subprocess module.
 */

#include <Python.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <hardware/atomic.h>

#define HANDLE_TO_PYNUM(handle) PyLong_FromUnsignedLong((long) handle)

struct StartupMsg {
    struct Message SysMsg;
    struct SignalSemaphore sem;
    struct Process *proc;
    struct Task *waiter;
    ULONG signal;
    LONG retcode;
};

static int
wait_for_stm(struct StartupMsg *stm, ULONG mask)
{
    BYTE breaksigbit;
    ULONG sig_break, sigs;
    struct Task *task;
    struct Process *proc;
    
    Py_BEGIN_ALLOW_THREADS
    ObtainSemaphore(&stm->sem);
    proc = stm->proc;
    ReleaseSemaphore(&stm->sem);
    Py_END_ALLOW_THREADS
    
    /* Already finished? */
    if (!proc)
        return 0;
        
    breaksigbit = AllocSignal(-1);
    if (breaksigbit < 0) {
        PyErr_SetFromMorphOSErr(IoErr());
        return -1;
    }
        
    task = FindTask(NULL);
    sig_break = 1 << breaksigbit;
    sigs = mask | sig_break;
    
    Py_BEGIN_ALLOW_THREADS
    
    ObtainSemaphore(&stm->sem);
    stm->signal = sig_break;
    stm->waiter = task;
    ReleaseSemaphore(&stm->sem);
    
    kprintf("_subprocess: wait...\n");
    sigs = Wait(sigs);
    kprintf("_subprocess: wait done, sigs=%X\n", sigs);
    
    ObtainSemaphore(&stm->sem);
    ATOMIC_STORE((ULONG *)&stm->waiter, NULL);
    ReleaseSemaphore(&stm->sem);
    
    FreeSignal(breaksigbit);

    /* I don't check the signal here
     * in case of non-waited Signal() call.
     */
    ObtainSemaphore(&stm->sem);
    proc = stm->proc;
    ReleaseSemaphore(&stm->sem);
    
    Py_END_ALLOW_THREADS
    
    if (!proc)
        return 1;
        
    return 0;
}

static void runcmd(CONST_STRPTR cmd, struct StartupMsg *stm)
{
    kprintf("_subprocess: cmd: [%s]\n", cmd);
    stm->retcode = SystemTags(cmd, TAG_DONE);
    kprintf("_subprocess: result=%ld for [%s]\n", stm->retcode, cmd);

    ObtainSemaphore(&stm->sem);
    stm->proc = NULL;
    if (stm->waiter)
        Signal(stm->waiter, stm->signal);
    ReleaseSemaphore(&stm->sem);
}

/* -------------------------------------------------------------------- */
/* BPTR handle wrapper */

typedef struct {
    PyObject_HEAD
    BPTR handle;
} sp_handle_object;

static PyTypeObject sp_handle_type;

static PyObject*
sp_handle_new(BPTR handle)
{
    sp_handle_object* self;

    self = PyObject_NEW(sp_handle_object, &sp_handle_type);
    if (self == NULL)
        return NULL;

    self->handle = handle;
    
    return (PyObject*) self;
}

static PyObject*
sp_handle_close(sp_handle_object* self, PyObject* args)
{
    if (! PyArg_ParseTuple(args, ":Close"))
        return NULL;

    if (self->handle != NULL) {
        UnLock(self->handle); /* identical to Close() on MorphOS only! */
        self->handle = NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef sp_handle_methods[] = {
    {"Close",  (PyCFunction) sp_handle_close,  METH_VARARGS},
    {NULL, NULL}
};

static PyObject*
sp_handle_getattr(sp_handle_object* self, char* name)
{
    return Py_FindMethod(sp_handle_methods, (PyObject*) self, name);
}

static PyObject*
sp_handle_as_long(sp_handle_object* self)
{
    return HANDLE_TO_PYNUM(self->handle);
}

static PyNumberMethods sp_handle_as_number = {
    .nb_long = (unaryfunc) sp_handle_as_long,
};

static PyTypeObject sp_handle_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /*ob_size*/
    "_subprocess_handle", sizeof(sp_handle_object),
    0,
    0,                                  /*tp_dealloc*/
    0,                                  /*tp_print*/
    (getattrfunc) sp_handle_getattr,    /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_compare*/
    0,                                  /*tp_repr*/
    &sp_handle_as_number,               /*tp_as_number */
    0,                                  /*tp_as_sequence */
    0,                                  /*tp_as_mapping */
    0                                   /*tp_hash*/
};

/* -------------------------------------------------------------------- */
/* MorphOS API functions */

PyDoc_STRVAR(GetStdHandle_doc,
"GetStdHandle(handle) -> integer\n\
\n\
Return a handle to the specified standard device\n\
(0=stdin, 1=stdout, 2=sterr).\n\
The integer associated with the handle object is returned.\n\
None is returned if nothing found.");

static PyObject *
sp_GetStdHandle(PyObject* self, PyObject* args)
{
    BPTR handle;
    int std_handle;

    if (! PyArg_ParseTuple(args, "i:GetStdHandle", &std_handle))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    SetIoErr(0);
    switch (std_handle) {
        case 0: handle = Input();
        case 1: handle = Output();
        case 2:
            {
                /* Try to obtain error handler from process (if not a task) */
                struct Process *proc = (struct Process *)FindTask(NULL);
                if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS)
                    handle = proc->pr_CES;
                else
                    handle = NULL;
                    
                /* Fallback on Output(), and duplicate the lock */
                if (handle == NULL)
                    handle = Output();
            }
            break;
        default:
            handle = NULL;
    }
    Py_END_ALLOW_THREADS

    if (IoErr())
        return PyErr_SetFromMorphOSErr(IoErr());

    if (!handle)
        Py_RETURN_NONE;

    /* Duplicate */
    Py_BEGIN_ALLOW_THREADS
    handle = OpenFromLock(DupLockFromFH(handle));
    Py_END_ALLOW_THREADS

    /* note: returns integer, not handle object */
    return HANDLE_TO_PYNUM(handle);
}

PyDoc_STRVAR(GetCurrentTask_doc,
"GetCurrentTask() -> long\n\
\n\
Return a long object for the current task pointer.");

static PyObject *
sp_GetCurrentTask(PyObject* self, PyObject* args)
{
    return PyLong_FromVoidPtr(FindTask(NULL));;
}

PyDoc_STRVAR(DuplicateHandle_doc,
"DuplicateHandle(source_handle) -> handle\n\
\n\
Return a duplicate handle object.\n\
\n\
The duplicate handle refers to the same object as the original\n\
handle. Therefore, any changes to the object are reflected\n\
through both handles.");

static PyObject *
sp_DuplicateHandle(PyObject* self, PyObject* args)
{
    BPTR source_handle, handle;

    if (!PyArg_ParseTuple(args, "l:DuplicateHandle", &source_handle))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    handle = OpenFromLock(DupLockFromFH(source_handle));
    Py_END_ALLOW_THREADS

    if (IoErr())
        return PyErr_SetFromMorphOSErr(IoErr());
        
    if (!handle)
        Py_RETURN_NONE;
        
    return sp_handle_new(handle);
}

PyDoc_STRVAR(ExecuteSubprocess_doc,
"ExecuteSubprocess() -> handle\n\
\n\
");

extern unsigned long __get_handle(int fd);

static PyObject *
sp_ExecuteSubprocess(PyObject* self, PyObject* args)
{
    PyObject *fd_in_o, *fd_out_o, *fd_err_o;
    BPTR fh_in, fh_out, fh_err;
    BOOL close_in, close_out, close_err;
    struct StartupMsg *stm;
    char *cmd;
    Py_ssize_t cmd_len;

    if (!PyArg_ParseTuple(args, "sOOO:ExecuteSubprocess", &cmd, &fd_in_o, &fd_out_o, &fd_err_o))
        return NULL;

    if (PyString_CheckExact(fd_in_o)) {
        Py_BEGIN_ALLOW_THREADS
        fh_in = Open(PyString_AS_STRING(fd_in_o), MODE_OLDFILE);
        Py_END_ALLOW_THREADS
        if (!fh_in)
            return PyErr_SetFromMorphOSErr(IoErr());
        close_in = TRUE;
    } else {
        fh_in = __get_handle(PyInt_AsLong(fd_in_o));
        close_in = FALSE;
    }
    
    if (PyString_CheckExact(fd_out_o)) {
        Py_BEGIN_ALLOW_THREADS
        fh_out = Open(PyString_AS_STRING(fd_out_o), MODE_NEWFILE);
        Py_END_ALLOW_THREADS
        if (!fh_out)
        {
            if (close_in)
                Close(fh_in);
            return PyErr_SetFromMorphOSErr(IoErr());
        }
        close_out = TRUE;
    } else {
        fh_out = __get_handle(PyInt_AsLong(fd_out_o));
        close_out = FALSE;
    }
        
    if (PyString_CheckExact(fd_err_o)) {
        Py_BEGIN_ALLOW_THREADS
        fh_err = Open(PyString_AS_STRING(fd_err_o), MODE_NEWFILE);
        Py_END_ALLOW_THREADS
        if (!fh_err)
        {
            if (close_in)
                Close(fh_in);
            if (close_out)
                Close(fh_out);
            return PyErr_SetFromMorphOSErr(IoErr());
        }
        close_err = TRUE;
    } else {
        fh_err = __get_handle(PyInt_AsLong(fd_err_o));
        close_err = FALSE;
    }
    
    if (!(fh_in && fh_out && fh_err)) {
        kprintf("_subprocess: fh_in=%p, fh_out=%p, fh_err=%p\n", fh_in, fh_out, fh_err);
        Py_RETURN_NONE;
    }
    
    cmd_len = strlen(cmd) + 1;
    stm = AllocVec(sizeof(struct StartupMsg) + cmd_len, MEMF_PUBLIC|MEMF_CLEAR);
    if (!stm)
        return PyErr_NoMemory();
        
    stm->SysMsg.mn_Length = sizeof(struct StartupMsg) + cmd_len;
    stm->SysMsg.mn_Node.ln_Type = NT_MESSAGE;
    InitSemaphore(&stm->sem);
    
    /* Detach command string from Python */
    CopyMem(cmd, &stm[1], cmd_len);

    stm->proc = CreateNewProcTags(
        NP_Entry,			(ULONG)runcmd,
        NP_PPC_Arg1,		(ULONG)&stm[1],
        NP_PPC_Arg2,		(ULONG)stm,
        NP_CodeType,		CODETYPE_PPC,
        NP_Name,			(ULONG)"_subprocess",
        NP_Priority,		0,
        NP_StartupMsg,		(ULONG)stm,
        NP_Cli,				TRUE,
        NP_CopyVars,		TRUE,
        NP_Input,			fh_in,
        NP_Output,			fh_out,
        NP_Error,			fh_err,
        NP_CloseInput,		close_in,
        NP_CloseOutput,		close_out,
        NP_CloseError,		close_err,
        //NP_CurrentDir,		(ULONG)"usr:",
        TAG_DONE);
        
    if (!stm->proc) {
        FreeVec(stm);
        return PyErr_NoMemory();
    }

    return Py_BuildValue("II", (unsigned int)stm->proc, (unsigned int)stm);
}

PyDoc_STRVAR(WaitForSTM_doc,
"WaitForSTM(stm, sigmask, timeout) -> result\n\
\n\
Wait until the specified task is in the signaled state or\n\
the time-out interval elapses. The timeout value is specified\n\
in milliseconds.");

static PyObject *
sp_WaitForSTM(PyObject* self, PyObject* args)
{
    struct StartupMsg *stm;
    ULONG mask;
    int res;
    
    if (!PyArg_ParseTuple(args, "kk:WaitForSTM", &stm, &mask))
        return NULL;

    res = wait_for_stm(stm, mask);
    if (res > 0)
        Py_RETURN_FALSE;
    else if (res < 0)
        return NULL;
    
    FreeVec(stm);
    return PyInt_FromLong(stm->retcode);
}

PyDoc_STRVAR(PollSTM_doc,
"PollSTM(stm) -> bool\n\
\n\
");

static PyObject *
sp_PollSTM(PyObject* self, PyObject* args)
{
    struct StartupMsg *stm;
    struct Process *proc;
    
    if (!PyArg_ParseTuple(args, "k:PollSTM", &stm))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ObtainSemaphore(&stm->sem);
    proc = stm->proc;
    ReleaseSemaphore(&stm->sem);
    Py_END_ALLOW_THREADS
    
    if (!proc)
        return PyInt_FromLong(stm->retcode);
        
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Terminate_doc,
"Terminate(stm) -> bool\n\
\n\
");

static PyObject *
sp_Terminate(PyObject* self, PyObject* args)
{
    struct StartupMsg *stm;
    struct Process *proc;
    
    if (!PyArg_ParseTuple(args, "k:Terminate", &stm))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ObtainSemaphore(&stm->sem);
    proc = stm->proc;
    if (proc)
        Signal(&stm->proc->pr_Task, SIGBREAKF_CTRL_C);
    ReleaseSemaphore(&stm->sem);
    Py_END_ALLOW_THREADS
    
    Py_RETURN_NONE;
}

static PyMethodDef sp_functions[] = {
    {"GetStdHandle", sp_GetStdHandle, METH_VARARGS, GetStdHandle_doc},
    {"DuplicateHandle", sp_DuplicateHandle, METH_VARARGS, DuplicateHandle_doc},
    {"GetCurrentTask", sp_GetCurrentTask, METH_NOARGS, GetCurrentTask_doc},
    {"ExecuteSubprocess", sp_ExecuteSubprocess, METH_VARARGS, ExecuteSubprocess_doc},
    {"WaitForSTM", sp_WaitForSTM, METH_VARARGS, WaitForSTM_doc},
    {"PollSTM", sp_PollSTM, METH_VARARGS, PollSTM_doc},
    {NULL, NULL}
};

static void
defint(PyObject* d, const char* name, int value)
{
    PyObject* v = PyInt_FromLong((long) value);
    if (v) {
        PyDict_SetItemString(d, (char*) name, v);
        Py_DECREF(v);
    }
}

PyMODINIT_FUNC
init_subprocess(void)
{
    PyObject *d;
    PyObject *m;

    sp_handle_type.ob_type = &PyType_Type;

    m = Py_InitModule("_subprocess", sp_functions);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);

    /* constants */
    defint(d, "SIGBREAKF_CTRL_C", SIGBREAKF_CTRL_C);
    defint(d, "SIGBREAKF_CTRL_D", SIGBREAKF_CTRL_D);
    defint(d, "SIGBREAKF_CTRL_E", SIGBREAKF_CTRL_E);
    defint(d, "SIGBREAKF_CTRL_F", SIGBREAKF_CTRL_F);
}