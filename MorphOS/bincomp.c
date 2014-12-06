#undef PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdarg.h>

int
PyArg_Parse(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = _PyArg_VaParseOld(args, format, va);
    va_end(va);
    return retval;
}

int
_PyArg_Parse_SizeT(PyObject *args, char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = _PyArg_VaParseOld_SizeT(args, format, va);
    va_end(va);
    return retval;
}

int
PyArg_ParseTuple(PyObject *args, const char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = PyArg_VaParse(args, format, va);
    va_end(va);
    return retval;
}

int
_PyArg_ParseTuple_SizeT(PyObject *args, char *format, ...)
{
    int retval;
    va_list va;

    va_start(va, format);
    retval = _PyArg_VaParse_SizeT(args, format, va);
    va_end(va);
    return retval;
}

int
PyArg_ParseTupleAndKeywords(PyObject *args,
                            PyObject *keywords,
                            const char *format,
                            char **kwlist, ...)
{
    int retval;
    va_list va;

    va_start(va, kwlist);
    retval = PyArg_VaParseTupleAndKeywords(args, keywords, format, kwlist, va);
    va_end(va);
    return retval;
}

int
_PyArg_ParseTupleAndKeywords_SizeT(PyObject *args,
                                   PyObject *keywords,
                                   const char *format,
                                   char **kwlist, ...)
{
    int retval;
    va_list va;

    va_start(va, kwlist);
    retval = _PyArg_VaParseTupleAndKeywords_SizeT(args, keywords, format, kwlist, va);
    va_end(va);
    return retval;
}

int
PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)
{
    Py_ssize_t i, l;
    PyObject **o;
    va_list vargs;

    va_start(vargs, max);
    assert(min >= 0);
    assert(min <= max);
    if (!PyTuple_Check(args)) {
        va_end(vargs);
        PyErr_SetString(PyExc_SystemError,
            "PyArg_UnpackTuple() argument list is not a tuple");
        return 0;
    }
    l = PyTuple_GET_SIZE(args);
    if (l < min) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%s expected %s%zd arguments, got %zd",
                name, (min == max ? "" : "at least "), min, l);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd elements,"
                " but has %zd",
                (min == max ? "" : "at least "), min, l);
        va_end(vargs);
        return 0;
    }
    if (l > max) {
        if (name != NULL)
            PyErr_Format(
                PyExc_TypeError,
                "%s expected %s%zd arguments, got %zd",
                name, (min == max ? "" : "at most "), max, l);
        else
            PyErr_Format(
                PyExc_TypeError,
                "unpacked tuple should have %s%zd elements,"
                " but has %zd",
                (min == max ? "" : "at most "), max, l);
        va_end(vargs);
        return 0;
    }
    for (i = 0; i < l; i++) {
        o = va_arg(vargs, PyObject **);
        *o = PyTuple_GET_ITEM(args, i);
    }
    va_end(vargs);
    return 1;
}

PyObject *
PyBytes_FromFormat(const char *format, ...)
{
    PyObject* ret;
    va_list vargs;

    va_start(vargs, format);
    ret = PyBytes_FromFormatV(format, vargs);
    va_end(vargs);
    return ret;
}

PyObject *
PyErr_Format(PyObject *exception, const char *format, ...)
{
    va_list vargs;
    PyObject* string;

    va_start(vargs, format);

#ifdef Py_DEBUG
    /* in debug mode, PyEval_EvalFrameEx() fails with an assertion error
       if an exception is set when it is called */
    PyErr_Clear();
#endif

    string = PyUnicode_FromFormatV(format, vargs);
    PyErr_SetObject(exception, string);
    Py_XDECREF(string);
    va_end(vargs);
    return NULL;
}

int
PyErr_WarnExplicitFormat(PyObject *category,
                         const char *filename_str, int lineno,
                         const char *module_str, PyObject *registry,
                         const char *format, ...)
{
    PyObject *message;
    PyObject *module = NULL;
    PyObject *filename = PyUnicode_DecodeFSDefault(filename_str);
    int ret = -1;
    va_list vargs;

    if (filename == NULL)
        goto exit;
    if (module_str != NULL) {
        module = PyUnicode_FromString(module_str);
        if (module == NULL)
            goto exit;
    }

    va_start(vargs, format);
    message = PyUnicode_FromFormatV(format, vargs);
    if (message != NULL) {
        ret = PyErr_WarnExplicitObject(category, message, filename, lineno,
									   module, registry);
        Py_DECREF(message);
    }
    va_end(vargs);
exit:
    Py_XDECREF(module);
    Py_XDECREF(filename);
    return ret;
}

int
PyErr_WarnFormat(PyObject *category, Py_ssize_t stack_level,
                 const char *format, ...)
{
	int retval;
    va_list va;

    va_start(va, format);
    retval = PyErr_VaWarnFormat(category, stack_level, format, va);
    va_end(va);
    return retval;
}

PyObject *
_PyErr_TrySetFromCause(const char *format, ...)
{
	va_list va;
	PyObject *retval;

    va_start(va, format);
    retval = _PyErr_VaTrySetFromCause(format, va);
    va_end(vargs);
	return retval;
}

PyObject *
PyEval_CallFunction(PyObject *obj, const char *format, ...)
{
    va_list vargs;
    PyObject *args;
    PyObject *res;

    va_start(vargs, format);
    args = Py_VaBuildValue(format, vargs);
    va_end(vargs);

    if (args == NULL)
        return NULL;

    res = PyEval_CallObject(obj, args);
    Py_DECREF(args);

    return res;
}

PyObject *
PyEval_CallMethod(PyObject *obj, const char *methodname, const char *format, ...)
{
    va_list vargs;
    PyObject *meth;
    PyObject *args;
    PyObject *res;

    meth = PyObject_GetAttrString(obj, methodname);
    if (meth == NULL)
        return NULL;

    va_start(vargs, format);

    args = Py_VaBuildValue(format, vargs);
    va_end(vargs);

    if (args == NULL) {
        Py_DECREF(meth);
        return NULL;
    }

    res = PyEval_CallObject(meth, args);
    Py_DECREF(meth);
    Py_DECREF(args);

    return res;
}

int
PyOS_snprintf(char *str, size_t size, const  char  *format, ...)
{
    int rc;
    va_list va;

    va_start(va, format);
    rc = PyOS_vsnprintf(str, size, format, va);
    va_end(va);
    return rc;
}

static PyObject *
type_error(const char *msg, PyObject *obj)
{
    PyErr_Format(PyExc_TypeError, msg, obj->ob_type->tp_name);
    return NULL;
}

static PyObject *
null_error(void)
{
    if (!PyErr_Occurred())
        PyErr_SetString(PyExc_SystemError,
                        "null argument to internal routine");
    return NULL;
}

static PyObject*
call_function_tail(PyObject *callable, PyObject *args)
{
    PyObject *retval;

    if (args == NULL)
        return NULL;

    if (!PyTuple_Check(args)) {
        PyObject *a;

        a = PyTuple_New(1);
        if (a == NULL) {
            Py_DECREF(args);
            return NULL;
        }
        PyTuple_SET_ITEM(a, 0, args);
        args = a;
    }
    retval = PyObject_Call(callable, args, NULL);

    Py_DECREF(args);

    return retval;
}

PyObject *
PyObject_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *args;

    if (callable == NULL)
        return null_error();

    if (format && *format) {
        va_start(va, format);
        args = Py_VaBuildValue(format, va);
        va_end(va);
    }
    else
        args = PyTuple_New(0);
    if (args == NULL)
        return NULL;

    return call_function_tail(callable, args);
}

PyObject *
_PyObject_CallFunction_SizeT(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *args;

    if (callable == NULL)
        return null_error();

    if (format && *format) {
        va_start(va, format);
        args = _Py_VaBuildValue_SizeT(format, va);
        va_end(va);
    }
    else
        args = PyTuple_New(0);

    return call_function_tail(callable, args);
}

static PyObject*
callmethod(PyObject* func, const char *format, va_list va, int is_size_t)
{
    PyObject *retval = NULL;
    PyObject *args;

    if (!PyCallable_Check(func)) {
        type_error("attribute of type '%.200s' is not callable", func);
        goto exit;
    }

    if (format && *format) {
        if (is_size_t)
            args = _Py_VaBuildValue_SizeT(format, va);
        else
            args = Py_VaBuildValue(format, va);
    }
    else
        args = PyTuple_New(0);

    retval = call_function_tail(func, args);

  exit:
    /* args gets consumed in call_function_tail */
    Py_XDECREF(func);

    return retval;
}

PyObject *
PyObject_CallMethod(PyObject *o, const char *name, const char *format, ...)
{
    va_list va;
    PyObject *func = NULL;
    PyObject *retval = NULL;

    if (o == NULL || name == NULL)
        return null_error();

    func = PyObject_GetAttrString(o, name);
    if (func == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(func, format, va, 0);
    va_end(va);
    return retval;
}

PyObject *
_PyObject_CallMethodId(PyObject *o, _Py_Identifier *name,
                       const char *format, ...)
{
    va_list va;
    PyObject *func = NULL;
    PyObject *retval = NULL;

    if (o == NULL || name == NULL)
        return null_error();

    func = _PyObject_GetAttrId(o, name);
    if (func == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(func, format, va, 0);
    va_end(va);
    return retval;
}

PyObject *
_PyObject_CallMethod_SizeT(PyObject *o, const char *name,
                           const char *format, ...)
{
    va_list va;
    PyObject *func = NULL;
    PyObject *retval;

    if (o == NULL || name == NULL)
        return null_error();

    func = PyObject_GetAttrString(o, name);
    if (func == NULL)
        return NULL;
    va_start(va, format);
    retval = callmethod(func, format, va, 1);
    va_end(va);
    return retval;
}

PyObject *
_PyObject_CallMethodId_SizeT(PyObject *o, _Py_Identifier *name,
                             const char *format, ...)
{
    va_list va;
    PyObject *func = NULL;
    PyObject *retval;

    if (o == NULL || name == NULL)
        return null_error();

    func = _PyObject_GetAttrId(o, name);
    if (func == NULL) {
        return NULL;
    }
    va_start(va, format);
    retval = callmethod(func, format, va, 1);
    va_end(va);
    return retval;
}

static PyObject *
objargs_mktuple(va_list va)
{
    int i, n = 0;
    va_list countva;
    PyObject *result, *tmp;

        Py_VA_COPY(countva, va);

    while (((PyObject *)va_arg(countva, PyObject *)) != NULL)
        ++n;
    result = PyTuple_New(n);
    if (result != NULL && n > 0) {
        for (i = 0; i < n; ++i) {
            tmp = (PyObject *)va_arg(va, PyObject *);
            PyTuple_SET_ITEM(result, i, tmp);
            Py_INCREF(tmp);
        }
    }
    return result;
}

PyObject *
PyObject_CallMethodObjArgs(PyObject *callable, PyObject *name, ...)
{
    PyObject *args, *tmp;
    va_list vargs;

    if (callable == NULL || name == NULL)
        return null_error();

    callable = PyObject_GetAttr(callable, name);
    if (callable == NULL)
        return NULL;

    /* count the args */
    va_start(vargs, name);
    args = objargs_mktuple(vargs);
    va_end(vargs);
    if (args == NULL) {
        Py_DECREF(callable);
        return NULL;
    }
    tmp = PyObject_Call(callable, args, NULL);
    Py_DECREF(args);
    Py_DECREF(callable);

    return tmp;
}

PyObject *
_PyObject_CallMethodIdObjArgs(PyObject *callable,
        struct _Py_Identifier *name, ...)
{
    PyObject *args, *tmp;
    va_list vargs;

    if (callable == NULL || name == NULL)
        return null_error();

    callable = _PyObject_GetAttrId(callable, name);
    if (callable == NULL)
        return NULL;

    /* count the args */
    va_start(vargs, name);
    args = objargs_mktuple(vargs);
    va_end(vargs);
    if (args == NULL) {
        Py_DECREF(callable);
        return NULL;
    }
    tmp = PyObject_Call(callable, args, NULL);
    Py_DECREF(args);
    Py_DECREF(callable);

    return tmp;
}

PyObject *
PyObject_CallFunctionObjArgs(PyObject *callable, ...)
{
    PyObject *args, *tmp;
    va_list vargs;

    if (callable == NULL)
        return null_error();

    /* count the args */
    va_start(vargs, callable);
    args = objargs_mktuple(vargs);
    va_end(vargs);
    if (args == NULL)
        return NULL;
    tmp = PyObject_Call(callable, args, NULL);
    Py_DECREF(args);

    return tmp;
}

static void
mywrite(char *name, FILE *fp, const char *format, va_list va)
{
    PyObject *file;
    PyObject *error_type, *error_value, *error_traceback;
    char buffer[1001];
    int written;

    PyErr_Fetch(&error_type, &error_value, &error_traceback);
    file = PySys_GetObject(name);
    written = PyOS_vsnprintf(buffer, sizeof(buffer), format, va);
    if (PyFile_WriteString(buffer, file) != 0) {
        PyErr_Clear();
        fputs(buffer, fp);
    }
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        const char *truncated = "... truncated";
        if (PyFile_WriteString(truncated, file) != 0) {
            PyErr_Clear();
            fputs(truncated, fp);
        }
    }
    PyErr_Restore(error_type, error_value, error_traceback);
}

void
PySys_WriteStdout(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    mywrite("stdout", stdout, format, va);
    va_end(va);
}

void
PySys_WriteStderr(const char *format, ...)
{
    va_list va;

    va_start(va, format);
    mywrite("stderr", stderr, format, va);
    va_end(va);
}

PyObject *
PyTuple_Pack(Py_ssize_t n, ...)
{
    Py_ssize_t i;
    PyObject *o;
    PyObject *result;
    PyObject **items;
    va_list vargs;

    va_start(vargs, n);
    result = PyTuple_New(n);
    if (result == NULL)
        return NULL;
    items = ((PyTupleObject *)result)->ob_item;
    for (i = 0; i < n; i++) {
        o = va_arg(vargs, PyObject *);
        Py_INCREF(o);
        items[i] = o;
    }
    va_end(vargs);
    return result;
}

PyObject *
PyUnicode_FromFormat(const char *format, ...)
{
    PyObject* ret;
    va_list vargs;

    va_start(vargs, format);
    ret = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    return ret;
}

PyObject *
Py_BuildValue(const char *format, ...)
{
    PyObject *retval;
    va_list va;

    va_start(va, format);
    retval = Py_VaBuildValue(format, va);
    va_end(va);
    return retval;
}

PyObject *
_Py_BuildValue_SizeT(const char *format, ...)
{
    PyObject *retval;
    va_list va;

    va_start(va, format);
    retval = _Py_VaBuildValue_SizeT(format, va);
    va_end(va);
    return retval;
}

