/* $Id:$
*/

#include <stdarg.h>

#undef PY_SSIZE_T_CLEAN

PyAPI_FUNC(int) _PyArg_VaParse_SizeT(PyObject *, char *, va_list);
PyAPI_FUNC(int) _PyArg_VaParseTupleAndKeywords_SizeT(PyObject *, PyObject *,
                                                     const char *, char **, va_list);

//+ PyArg_ParseTuple
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
//-
//+ _PyArg_ParseTuple_SizeT
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
//-
//+ PyArg_ParseTupleAndKeywords
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
//-
//+ _PyArg_ParseTupleAndKeywords_SizeT
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
//-
//+ Py_BuildValue
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
//-
//+ _Py_BuildValue_SizeT
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
//-
//+ PyEval_CallFunction
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
//-
//+ PyEval_CallMethod
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
//-

//+ PyTuple_Pack
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
//-

//+ mywrite
static void
mywrite(char *name, FILE *fp, const char *format, va_list va)
{
	PyObject *file;
	PyObject *error_type, *error_value, *error_traceback;

	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	file = PySys_GetObject(name);
	if (file == NULL || PyFile_AsFile(file) == fp)
		vfprintf(fp, format, va);
	else {
		char buffer[1001];
		const int written = PyOS_vsnprintf(buffer, sizeof(buffer),
						   format, va);
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
	}
	PyErr_Restore(error_type, error_value, error_traceback);
}
//-
//+ PySys_WriteStdout
void
PySys_WriteStdout(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	mywrite("stdout", stdout, format, va);
	va_end(va);
}
//-
//+ PySys_WriteStderr
void
PySys_WriteStderr(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	mywrite("stderr", stderr, format, va);
	va_end(va);
}
//-

//+ PyErr_Format
PyObject *
PyErr_Format(PyObject *exception, const char *format, ...)
{
	va_list vargs;
	PyObject* string;

	va_start(vargs, format);
	string = PyString_FromFormatV(format, vargs);
	PyErr_SetObject(exception, string);
	Py_XDECREF(string);
	va_end(vargs);
	return NULL;
}
//-

//+ PyOS_snprintf
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
//-

//+ PyArg_UnpackTuple
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
//-

//+ type_error
static PyObject *
type_error(const char *msg, PyObject *obj)
{
	PyErr_Format(PyExc_TypeError, msg, obj->ob_type->tp_name);
	return NULL;
}
//-
//+ null_error
static PyObject *
null_error(void)
{
	if (!PyErr_Occurred())
		PyErr_SetString(PyExc_SystemError,
				"null argument to internal routine");
	return NULL;
}
//-
//+ call_function_tail
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
//-
//+ objargs_mktuple
static PyObject *
objargs_mktuple(va_list va)
{
	int i, n = 0;
	va_list countva;
	PyObject *result, *tmp;

#ifdef __va_copy
	__va_copy(countva, va);
#else
	countva = va;
#endif

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
//-
//+ PyObject_CallFunction
PyObject *
PyObject_CallFunction(PyObject *callable, char *format, ...)
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

	return call_function_tail(callable, args);
}
//-
//+ _PyObject_CallFunction_SizeT
PyObject *
_PyObject_CallFunction_SizeT(PyObject *callable, char *format, ...)
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
//-
//+ PyObject_CallMethod
PyObject *
PyObject_CallMethod(PyObject *o, char *name, char *format, ...)
{
	va_list va;
	PyObject *args;
	PyObject *func = NULL;
	PyObject *retval = NULL;

	if (o == NULL || name == NULL)
		return null_error();

	func = PyObject_GetAttrString(o, name);
	if (func == NULL) {
		PyErr_SetString(PyExc_AttributeError, name);
		return 0;
	}

	if (!PyCallable_Check(func)) {
		type_error("attribute of type '%.200s' is not callable", func);
		goto exit;
	}

	if (format && *format) {
		va_start(va, format);
		args = Py_VaBuildValue(format, va);
		va_end(va);
	}
	else
		args = PyTuple_New(0);

	retval = call_function_tail(func, args);

  exit:
	/* args gets consumed in call_function_tail */
	Py_XDECREF(func);

	return retval;
}
//-
//+ _PyObject_CallMethod_SizeT
PyObject *
_PyObject_CallMethod_SizeT(PyObject *o, char *name, char *format, ...)
{
	va_list va;
	PyObject *args;
	PyObject *func = NULL;
	PyObject *retval = NULL;

	if (o == NULL || name == NULL)
		return null_error();

	func = PyObject_GetAttrString(o, name);
	if (func == NULL) {
		PyErr_SetString(PyExc_AttributeError, name);
		return 0;
	}

	if (!PyCallable_Check(func)) {
		type_error("attribute of type '%.200s' is not callable", func);
		goto exit;
	}

	if (format && *format) {
		va_start(va, format);
		args = _Py_VaBuildValue_SizeT(format, va);
		va_end(va);
	}
	else
		args = PyTuple_New(0);

	retval = call_function_tail(func, args);

  exit:
	/* args gets consumed in call_function_tail */
	Py_XDECREF(func);

	return retval;
}
//-
//+ PyObject_CallMethodObjArgs
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
//-
//+ PyObject_CallFunctionObjArgs
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
//-

//+ PyString_FromFormat
PyObject *
PyString_FromFormat(const char *format, ...)
{
	PyObject* ret;
	va_list vargs;

	va_start(vargs, format);
	ret = PyString_FromFormatV(format, vargs);
	va_end(vargs);
	return ret;
}
//-
