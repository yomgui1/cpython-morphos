#include "Python.h"

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#ifndef MODNAME
#define MODNAME "doslib"
#endif

#ifndef INITFUNC
#define INITFUNC initdoslib
#endif

#define INSI(m, s, v) if (PyModule_AddIntConstant(m, s, v)) return -1
#define INSS(m, s, v) if (PyModule_AddStringConstant(m, s, v)) return -1
#define INSL(m, s, v) if (PyModule_AddObject(m, s, PyLong_FromUnsignedLong(v))) return -1

PyDoc_STRVAR(module_doc,
"Python wrapper on AmigaDOS library.");

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
    Py_END_ALLOW_THREADS
    
    if (NULL == fh) {
        LONG err = IoErr();

        if (err != 0) {
            char buf[82];

            Fault(err, "'", buf, sizeof(buf));
            PyErr_Format(PyExc_OSError, "Open() failed, DOS error #%lu%s'.", err, buf);
        } else
            PyErr_SetString(PyExc_OSError, "Open() failed, but with 0 DOS error.");

        return NULL;
    }

    return PyInt_FromLong((long)fh);
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

    if (NULL == fh) {
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
//+ doslib_CreateNewProc
static PyObject *
doslib_CreateNewProc(PyObject *self, PyObject *args)
{
    BPTR fh_in, fh_out, fh_err;
    STRPTR cmd, cwd;
    struct Process *proc;

    if (!PyArg_ParseTuple(args, "sziii", &cmd, &cwd, &fh_in, &fh_out, &fh_err))
        return NULL;

    if ((NULL == fh_in) || (NULL == fh_out) || (NULL == fh_err)) {
        PyErr_SetString(PyExc_ValueError, "Bad file handle value.");
        return NULL;
    }

    if (NULL == cwd)
        cwd = "";

    /*proc = SystemTags(cmd,
        SYS_Input,      (ULONG)fh_in,
        SYS_Output,     (ULONG)fh_out,

        NP_CurrentDir,  (ULONG)cwd,
        NP_Error,       (ULONG)fh_err,
        NP_CloseError,  FALSE,
        TAG_END);*/

    if (NULL == proc) {
        LONG err = IoErr();
        if (err != 0) {
            char buf[82];

            Fault(err, "'", buf, sizeof(buf));
            PyErr_Format(PyExc_OSError, "CreateNewProc() failed, DOS error #%lu%s'.", err, buf);
        }  else
            PyErr_SetString(PyExc_OSError, "CreateNewProc() failed, but with 0 DOS error.");

        return NULL;
    }

    return PyLong_FromVoidPtr(proc);
}
//-

//+ module_methods
static PyMethodDef module_methods[] = {
    {"Open", doslib_open, METH_VARARGS,
        PyDoc_STR("Open(name, mode) -> OS file handle")},
    {"Close", doslib_close, METH_VARARGS,
        PyDoc_STR("Close(fh) -> None")},
    {"CreateNewProc", doslib_CreateNewProc, METH_VARARGS,
        PyDoc_STR("CreateNewProc(stdin, stdout, stderr) -> None")},
    {NULL,	    NULL}	    /* sentinel */
};
//-

//+ all_ins
static int
all_ins(PyObject *m) {
    INSI(m, "MODE_OLDFILE", MODE_OLDFILE);
    INSI(m, "MODE_NEWFILE", MODE_NEWFILE);
    INSI(m, "MODE_READWRITE", MODE_READWRITE);

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
