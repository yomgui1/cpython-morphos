#include "Python.h"

#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "mosdebug.h"

#ifndef MODNAME
#define MODNAME "doslib"
#endif

#ifndef INITFUNC
#define INITFUNC initdoslib
#endif

#define INSI(m, s, v) if (PyModule_AddIntConstant(m, s, v)) return -1
#define INSS(m, s, v) if (PyModule_AddStringConstant(m, s, v)) return -1
#define INSL(m, s, v) if (PyModule_AddObject(m, s, PyLong_FromUnsignedLong(v))) return -1

#define SUBTASK_STACKSIZE   16384
#define SUBTASK_PRIORITY    0
#define SUBTASK_NAME        "Python [Popen subtask]"

PyDoc_STRVAR(module_doc,
"Python wrapper on AmigaDOS library.");

struct ExecTaskMsg {
    struct Message  msg;
    struct Process *proc;
    LONG            result;
    LONG            error;
    BPTR            hin, hout, herr;
    STRPTR          cmd;
};

//+ execute_task
void execute_task(void)
{
    struct ExecBase * SysBase = *(APTR *)4L;
    struct DosLibrary * DOSBase;
    struct ExecTaskMsg *stm;
    struct Process *self = (APTR) FindTask(NULL);

    DPRINT("<execute_task %p>: launched\n", self);

    DOSBase = (APTR)OpenLibrary("dos.library", 36);
    if (DOSBase != NULL)
    {
        LONG res;

        res = NewGetTaskAttrs(NULL, &stm, sizeof(struct ExecTaskMsg *), TASKINFOTYPE_STARTUPMSG, TAG_DONE);
        if (res && (NULL != stm))
        {
            BPTR hin, hout, herr;

            DPRINT("<execute_task %p>: stm FH (%p, %p, %p)\n", self, stm->hin, stm->hout, stm->herr);

            if (stm->hin) {
                hin = DupLockFromFH(stm->hin);
                DPRINT("dup hin: %p\n", hin);
            } else
                hin = 0;

            if (stm->hout) {
                hout = DupLockFromFH(stm->hout);
                DPRINT("dup hout: %p\n", hout);
            } else
                hout = 0;

            if (stm->herr) {
                herr = DupLockFromFH(stm->hout);
                DPRINT("dup herr: %p\n", herr);
            } else
                herr = 0;

            DPRINT("<execute_task %p>: used FH (%p, %p, %p)\n", self, hin, hout, herr);

            // execute command synchronously
            DPRINT("run command: '%s'\n", stm->cmd);
            stm->result = SystemTags(stm->cmd,
                                     SYS_Input, hin,
                                     SYS_Output, hout,
                                     NP_Error, herr,
                                     NP_CloseError, FALSE,
                                     TAG_DONE);
            stm->error = IoErr();
            DPRINT("<execute_task %p>: result=%ld, error=%ld\n", self, stm->result, stm->error);

            FreeVec(stm->cmd);

        bye:
            if (hin) Close(hin);
            if (hout) Close(hout);
            if (herr) Close(herr);
        }
        else
            DPRINT_ERROR("<execute_task %p>: can't get startup msg\n", self);
 
        CloseLibrary((APTR)DOSBase);
    }
    else
        DPRINT_ERROR("<execute_task %p>: can't open dos.library >= 36\n", self);

    DPRINT("<execute_task %p>: Bye\n", self);
}
//-

//+ doslib_open
static PyObject *
doslib_open(PyObject *self, PyObject *args)
{
    BPTR fh;
    STRPTR name;
    LONG mode;

    if (!PyArg_ParseTuple(args, "si", &name, &mode))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    fh = Open(name, mode);
    DPRINT("Open('%s', %lu) => %p\n", name, mode, fh);
    Py_END_ALLOW_THREADS
    
    if (!fh) {
        LONG err = IoErr();

        if (err != 0) {
            char buf[82];

            Fault(err, "'", buf, sizeof(buf));
            PyErr_Format(PyExc_OSError, "Open() failed, DOS error #%lu%s'.", err, buf);
        } else
            PyErr_SetString(PyExc_OSError, "Open() failed, but with 0 DOS error.");

        return NULL;
    }

    return PyInt_FromLong(fh);
}
//-
//+ doslib_close
static PyObject *
doslib_close(PyObject *self, PyObject *args)
{
    BPTR fh;
    LONG err;

    if (!PyArg_ParseTuple(args, "i", &fh))
        return NULL;

    if (!fh) {
        PyErr_SetString(PyExc_ValueError, "Bad file handle.");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    Close(fh);
    Py_END_ALLOW_THREADS

    err = IoErr();
    if (err != 0) {
        char buf[82];

        Fault(err, "'", buf, sizeof(buf));
        return PyErr_Format(PyExc_OSError, "Close() failed, DOS error #%lu%s'.", err, buf);
    }

    Py_RETURN_NONE;
}
//-
//+ doslib_async_execute
static PyObject *
doslib_async_execute(PyObject *self, PyObject *args)
{
    BPTR hin, hout, herr;
    STRPTR cmd, cmd2, cwd;
    struct ExecTaskMsg *stm;

    if (!PyArg_ParseTuple(args, "sziii", &cmd, &cwd, &hin, &hout, &herr))
        return NULL;

    DPRINT("FH (%p, %p, %p)\n", hin, hout, herr);
    
    if (0 == strlen(cmd))
        return PyErr_Format(PyExc_ValueError, "cmd is empty");

    cmd2 = AllocVec(strlen(cmd), MEMF_PUBLIC);
    DPRINT("cmd2=%p\n", cmd2);
    if (NULL == cmd2)
        return PyErr_NoMemory();
    strcpy(cmd2, cmd);

    stm = AllocMem(sizeof(*stm), MEMF_PUBLIC | MEMF_CLEAR);
    DPRINT("stm=%p\n", stm);
    if (NULL == stm) {
        FreeVec(cmd2);
        return PyErr_NoMemory();
    }

    stm->msg.mn_ReplyPort = CreateMsgPort();
    DPRINT("rport=%p\n", stm->msg.mn_ReplyPort);
    if (NULL == stm->msg.mn_ReplyPort) {
        FreeMem(stm, sizeof(*stm));
        FreeVec(cmd2);
        return PyErr_NoMemory();
    }
    stm->msg.mn_Node.ln_Type = NT_MESSAGE;
    stm->msg.mn_Length = sizeof(*stm);

    stm->hin = hin;
    stm->hout = hout;
    stm->herr = herr;
    stm->result = -1;
    stm->cmd = cmd2;

    stm->proc = CreateNewProcTags(
        NP_CodeType,        CODETYPE_PPC,
        NP_Entry,           (ULONG) execute_task,
        NP_StartupMsg,      (ULONG) stm,
        NP_PPCStackSize,    SUBTASK_STACKSIZE,
        NP_Priority,        SUBTASK_PRIORITY,
        NP_Name,            (ULONG) SUBTASK_NAME,
        NP_Cli,             TRUE,
        NP_CurrentDir,      (ULONG) cwd,
        TAG_DONE);

    if (NULL == stm->proc) {
        LONG err = IoErr();

        if (err != 0) {
            char buf[82];

            Fault(err, "'", buf, sizeof(buf));
            PyErr_Format(PyExc_OSError, "CreateNewProc() failed, DOS error #%lu%s'.", err, buf);
        }  else
            PyErr_SetString(PyExc_OSError, "CreateNewProc() failed, but with 0 DOS error.");

        FreeVec(cmd2);
        DeleteMsgPort(stm->msg.mn_ReplyPort);
        FreeMem(stm, sizeof(*stm));
        return NULL;
    }

    return Py_BuildValue("ii", (int) stm, (int) stm->proc);
}
//-
//+ doslib_wait_proc
static PyObject *
doslib_wait_proc(PyObject *self, PyObject *args)
{
    struct ExecTaskMsg *stm;
    struct Message *msg;
    LONG timeout = 0;

    if (!PyArg_ParseTuple(args, "i|i", &stm, &timeout))
        return NULL;

    if ((NULL == stm) || !TypeOfMem(stm)) {
        PyErr_SetString(PyExc_ValueError, "invalid OS process handle");
        return NULL;
    }

    msg = GetMsg(stm->msg.mn_ReplyPort);
    if (NULL == msg) {
        if (0 != timeout) {
            /* TODO: replace me with a real code, using timeout! */
            WaitPort(stm->msg.mn_ReplyPort);
        } else
            Py_RETURN_NONE;
    } else {
        DeleteMsgPort(stm->msg.mn_ReplyPort);
        FreeVec(stm->cmd);
        FreeMem(stm, sizeof(*stm));
    }

    return Py_BuildValue("ii", stm->result, stm->error);
}
//-
//+ doslib_change_mode
static PyObject *
doslib_change_mode(PyObject *self, PyObject *args)
{
    BPTR object;
    ULONG type, newmode;

    if (!PyArg_ParseTuple(args, "III", &type, &object, &newmode))
        return NULL;

    if (0 == object) {
        PyErr_SetString(PyExc_ValueError, "NULL object");
        return NULL;
    }

    return Py_BuildValue("b", ChangeMode(type, object, newmode));
}
//-

static PyMethodDef module_methods[] = {
    {"open", doslib_open, METH_VARARGS,
     PyDoc_STR("open(name, mode) -> OS file handle")},
    {"close", doslib_close, METH_VARARGS,
     PyDoc_STR("close(fh) -> None")},
    {"async_execute", doslib_async_execute, METH_VARARGS,
     PyDoc_STR("async_execute(cmd, cwd, stdin, stdout, stderr) -> OS Process handle")},
    {"wait_proc", doslib_wait_proc, METH_VARARGS,
     PyDoc_STR("wait_proc(handle, block=0) -> process result")},
    {"ChangeMode", doslib_change_mode, METH_VARARGS,
     PyDoc_STR("ChangeMode(type, handle, mode) -> result (bool)")},
    {NULL, NULL} /* sentinel */
};

//+ all_ins
static int
all_ins(PyObject *m) {
    INSI(m, "MODE_OLDFILE", MODE_OLDFILE);
    INSI(m, "MODE_NEWFILE", MODE_NEWFILE);
    INSI(m, "MODE_READWRITE", MODE_READWRITE);
    INSI(m, "CHANGE_FH", CHANGE_FH);
    INSI(m, "CHANGE_LOCK", CHANGE_LOCK);

    return 0;
}
//- all_ins

PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m;

    m = Py_InitModule3(MODNAME, module_methods, module_doc);
    if (NULL == m) return;
    if (all_ins(m)) return;
}
