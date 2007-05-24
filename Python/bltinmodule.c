/* Built-in functions */

#include "Python.h"

#include "node.h"
#include "code.h"
#include "eval.h"

#include <ctype.h>

#ifdef RISCOS
#include "unixstuff.h"
#endif

/* The default encoding used by the platform file system APIs
   Can remain NULL for all platforms that don't have such a concept
*/
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
const char *Py_FileSystemDefaultEncoding = "mbcs";
#elif defined(__APPLE__)
const char *Py_FileSystemDefaultEncoding = "utf-8";
#else
const char *Py_FileSystemDefaultEncoding = NULL; /* use default */
#endif

/* Forward */
static PyObject *filterstring(PyObject *, PyObject *);
static PyObject *filterunicode(PyObject *, PyObject *);
static PyObject *filtertuple (PyObject *, PyObject *);

static PyObject *
builtin___build_class__(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *func, *name, *bases, *mkw, *meta, *prep, *ns, *res;
	Py_ssize_t nargs, nbases;

	assert(args != NULL);
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
				"__build_class__: args is not a tuple");
		return NULL;
	}
	nargs = PyTuple_GET_SIZE(args);
	if (nargs < 2) {
		PyErr_SetString(PyExc_TypeError,
				"__build_class__: not enough arguments");
		return NULL;
	}
	func = PyTuple_GET_ITEM(args, 0); /* Better be callable */
	name = PyTuple_GET_ITEM(args, 1);
	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"__build_class__: name is not a string");
		return NULL;
	}
	bases = PyTuple_GetSlice(args, 2, nargs);
	if (bases == NULL)
		return NULL;
	nbases = nargs - 2;

	if (kwds == NULL) {
		meta = NULL;
                mkw = NULL;
        }
	else {
		mkw = PyDict_Copy(kwds); /* Don't modify kwds passed in! */
		if (mkw == NULL) {
			Py_DECREF(bases);
			return NULL;
		}
		meta = PyDict_GetItemString(mkw, "metaclass");
		if (meta != NULL) {
			Py_INCREF(meta);
			if (PyDict_DelItemString(mkw, "metaclass") < 0) {
				Py_DECREF(meta);
				Py_DECREF(mkw);
				Py_DECREF(bases);
				return NULL;
			}
		}
	}
	if (meta == NULL) {
		if (PyTuple_GET_SIZE(bases) == 0)
			meta = (PyObject *) (&PyType_Type);
		else {
			PyObject *base0 = PyTuple_GET_ITEM(bases, 0);
			meta = (PyObject *) (base0->ob_type);
		}
		Py_INCREF(meta);
	}
	prep = PyObject_GetAttrString(meta, "__prepare__");
	if (prep == NULL) {
		PyErr_Clear();
		ns = PyDict_New();
	}
	else {
		PyObject *pargs = Py_BuildValue("OO", name, bases);
		if (pargs == NULL) {
			Py_DECREF(prep);
			Py_DECREF(meta);
			Py_XDECREF(mkw);
			Py_DECREF(bases);
			return NULL;
		}
		ns = PyEval_CallObjectWithKeywords(prep, pargs, mkw);
		Py_DECREF(pargs);
		Py_DECREF(prep);
		if (ns == NULL) {
			Py_DECREF(meta);
			Py_XDECREF(mkw);
			Py_DECREF(bases);
			return NULL;
		}
	}
	res = PyObject_CallFunctionObjArgs(func, ns, NULL);
	if (res != NULL) {
		PyObject *margs;
		Py_DECREF(res);
		res = NULL;
		margs = Py_BuildValue("OOO", name, bases, ns);
		if (margs != NULL) {
			res = PyEval_CallObjectWithKeywords(meta, margs, mkw);
			Py_DECREF(margs);
		}
	}
	Py_DECREF(ns);
	Py_DECREF(meta);
	Py_XDECREF(mkw);
	Py_DECREF(bases);
	return res;
}

PyDoc_STRVAR(build_class_doc,
"__build_class__(func, name, *bases, metaclass=None, **kwds) -> class\n\
\n\
Internal helper function used by the class statement.");

static PyObject *
builtin___import__(PyObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"name", "globals", "locals", "fromlist",
				 "level", 0};
	char *name;
	PyObject *globals = NULL;
	PyObject *locals = NULL;
	PyObject *fromlist = NULL;
	int level = -1;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|OOOi:__import__",
			kwlist, &name, &globals, &locals, &fromlist, &level))
		return NULL;
	return PyImport_ImportModuleLevel(name, globals, locals,
					  fromlist, level);
}

PyDoc_STRVAR(import_doc,
"__import__(name, globals={}, locals={}, fromlist=[], level=-1) -> module\n\
\n\
Import a module.  The globals are only used to determine the context;\n\
they are not modified.  The locals are currently unused.  The fromlist\n\
should be a list of names to emulate ``from name import ...'', or an\n\
empty list to emulate ``import name''.\n\
When importing a module from a package, note that __import__('A.B', ...)\n\
returns package A when fromlist is empty, but its submodule B when\n\
fromlist is not empty.  Level is used to determine whether to perform \n\
absolute or relative imports.  -1 is the original strategy of attempting\n\
both absolute and relative imports, 0 is absolute, a positive number\n\
is the number of parent directories to search relative to the current module.");


static PyObject *
builtin_abs(PyObject *self, PyObject *v)
{
	return PyNumber_Absolute(v);
}

PyDoc_STRVAR(abs_doc,
"abs(number) -> number\n\
\n\
Return the absolute value of the argument.");

static PyObject *
builtin_all(PyObject *self, PyObject *v)
{
	PyObject *it, *item;

	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;

	while ((item = PyIter_Next(it)) != NULL) {
		int cmp = PyObject_IsTrue(item);
		Py_DECREF(item);
		if (cmp < 0) {
			Py_DECREF(it);
			return NULL;
		}
		if (cmp == 0) {
			Py_DECREF(it);
			Py_RETURN_FALSE;
		}
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(all_doc,
"all(iterable) -> bool\n\
\n\
Return True if bool(x) is True for all values x in the iterable.");

static PyObject *
builtin_any(PyObject *self, PyObject *v)
{
	PyObject *it, *item;

	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;

	while ((item = PyIter_Next(it)) != NULL) {
		int cmp = PyObject_IsTrue(item);
		Py_DECREF(item);
		if (cmp < 0) {
			Py_DECREF(it);
			return NULL;
		}
		if (cmp == 1) {
			Py_DECREF(it);
			Py_RETURN_TRUE;
		}
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_FALSE;
}

PyDoc_STRVAR(any_doc,
"any(iterable) -> bool\n\
\n\
Return True if bool(x) is True for any x in the iterable.");


static PyObject *
builtin_filter(PyObject *self, PyObject *args)
{
	PyObject *func, *seq, *result, *it, *arg;
	Py_ssize_t len;   /* guess for result list size */
	register Py_ssize_t j;

	if (!PyArg_UnpackTuple(args, "filter", 2, 2, &func, &seq))
		return NULL;

	/* Strings and tuples return a result of the same type. */
	if (PyString_Check(seq))
		return filterstring(func, seq);
	if (PyUnicode_Check(seq))
		return filterunicode(func, seq);
	if (PyTuple_Check(seq))
		return filtertuple(func, seq);

	/* Pre-allocate argument list tuple. */
	arg = PyTuple_New(1);
	if (arg == NULL)
		return NULL;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		goto Fail_arg;

	/* Guess a result list size. */
	len = _PyObject_LengthHint(seq);
	if (len < 0) {
		if (!PyErr_ExceptionMatches(PyExc_TypeError)  &&
		    !PyErr_ExceptionMatches(PyExc_AttributeError)) {
			goto Fail_it;
		}
		PyErr_Clear();
		len = 8;	/* arbitrary */
	}

	/* Get a result list. */
	if (PyList_Check(seq) && seq->ob_refcnt == 1) {
		/* Eww - can modify the list in-place. */
		Py_INCREF(seq);
		result = seq;
	}
	else {
		result = PyList_New(len);
		if (result == NULL)
			goto Fail_it;
	}

	/* Build the result list. */
	j = 0;
	for (;;) {
		PyObject *item;
		int ok;

		item = PyIter_Next(it);
		if (item == NULL) {
			if (PyErr_Occurred())
				goto Fail_result_it;
			break;
		}

		if (func == (PyObject *)&PyBool_Type || func == Py_None) {
			ok = PyObject_IsTrue(item);
		}
		else {
			PyObject *good;
			PyTuple_SET_ITEM(arg, 0, item);
			good = PyObject_Call(func, arg, NULL);
			PyTuple_SET_ITEM(arg, 0, NULL);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_result_it;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			if (j < len)
				PyList_SET_ITEM(result, j, item);
			else {
				int status = PyList_Append(result, item);
				Py_DECREF(item);
				if (status < 0)
					goto Fail_result_it;
			}
			++j;
		}
		else
			Py_DECREF(item);
	}


	/* Cut back result list if len is too big. */
	if (j < len && PyList_SetSlice(result, j, len, NULL) < 0)
		goto Fail_result_it;

	Py_DECREF(it);
	Py_DECREF(arg);
	return result;

Fail_result_it:
	Py_DECREF(result);
Fail_it:
	Py_DECREF(it);
Fail_arg:
	Py_DECREF(arg);
	return NULL;
}

PyDoc_STRVAR(filter_doc,
"filter(function or None, sequence) -> list, tuple, or string\n"
"\n"
"Return those items of sequence for which function(item) is true.  If\n"
"function is None, return the items that are true.  If sequence is a tuple\n"
"or string, return the same type, else return a list.");


static PyObject *
builtin_chr(PyObject *self, PyObject *args)
{
	long x;
	char s[1];

	if (!PyArg_ParseTuple(args, "l:chr8", &x))
		return NULL;
	if (x < 0 || x >= 256) {
		PyErr_SetString(PyExc_ValueError,
				"chr8() arg not in range(256)");
		return NULL;
	}
	s[0] = (char)x;
	return PyString_FromStringAndSize(s, 1);
}

PyDoc_STRVAR(chr_doc,
"chr8(i) -> 8-bit character\n\
\n\
Return a string of one character with ordinal i; 0 <= i < 256.");


static PyObject *
builtin_unichr(PyObject *self, PyObject *args)
{
	long x;

	if (!PyArg_ParseTuple(args, "l:unichr", &x))
		return NULL;

	return PyUnicode_FromOrdinal(x);
}

PyDoc_STRVAR(unichr_doc,
"chr(i) -> Unicode character\n\
\n\
Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");


static PyObject *
builtin_cmp(PyObject *self, PyObject *args)
{
	PyObject *a, *b;
	int c;

	if (!PyArg_UnpackTuple(args, "cmp", 2, 2, &a, &b))
		return NULL;
	if (PyObject_Cmp(a, b, &c) < 0)
		return NULL;
	return PyInt_FromLong((long)c);
}

PyDoc_STRVAR(cmp_doc,
"cmp(x, y) -> integer\n\
\n\
Return negative if x<y, zero if x==y, positive if x>y.");


static char *
source_as_string(PyObject *cmd)
{
	char *str;
	Py_ssize_t size;

	if (!PyObject_CheckReadBuffer(cmd) &&
	    !PyUnicode_Check(cmd)) {
		PyErr_SetString(PyExc_TypeError,
			   "eval()/exec() arg 1 must be a string, bytes or code object");
		return NULL;
	}

	if (PyUnicode_Check(cmd)) {
		cmd = _PyUnicode_AsDefaultEncodedString(cmd, NULL);
		if (cmd == NULL)
			return NULL;
	}
	if (PyObject_AsReadBuffer(cmd, (const void **)&str, &size) < 0) {
		return NULL;
	}
	if (strlen(str) != size) {
		PyErr_SetString(PyExc_TypeError,
				"source code string cannot contain null bytes");
		return NULL;
	}
	return str;
}

static PyObject *
builtin_compile(PyObject *self, PyObject *args, PyObject *kwds)
{
	char *str;
	char *filename;
	char *startstr;
	int start;
	int dont_inherit = 0;
	int supplied_flags = 0;
	PyCompilerFlags cf;
	PyObject *cmd;
	static char *kwlist[] = {"source", "filename", "mode", "flags",
				 "dont_inherit", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oss|ii:compile",
					 kwlist, &cmd, &filename, &startstr,
					 &supplied_flags, &dont_inherit))
		return NULL;

	cf.cf_flags = supplied_flags | PyCF_SOURCE_IS_UTF8;

	str = source_as_string(cmd);
	if (str == NULL)
		return NULL;

	if (strcmp(startstr, "exec") == 0)
		start = Py_file_input;
	else if (strcmp(startstr, "eval") == 0)
		start = Py_eval_input;
	else if (strcmp(startstr, "single") == 0)
		start = Py_single_input;
	else {
		PyErr_SetString(PyExc_ValueError,
		   "compile() arg 3 must be 'exec' or 'eval' or 'single'");
		return NULL;
	}

	if (supplied_flags &
	    ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_DONT_IMPLY_DEDENT | PyCF_ONLY_AST))
	{
		PyErr_SetString(PyExc_ValueError,
				"compile(): unrecognised flags");
		return NULL;
	}
	/* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

	if (!dont_inherit) {
		PyEval_MergeCompilerFlags(&cf);
	}
	return Py_CompileStringFlags(str, filename, start, &cf);
}

PyDoc_STRVAR(compile_doc,
"compile(source, filename, mode[, flags[, dont_inherit]]) -> code object\n\
\n\
Compile the source string (a Python module, statement or expression)\n\
into a code object that can be executed by exec() or eval().\n\
The filename will be used for run-time error messages.\n\
The mode must be 'exec' to compile a module, 'single' to compile a\n\
single (interactive) statement, or 'eval' to compile an expression.\n\
The flags argument, if present, controls which future statements influence\n\
the compilation of the code.\n\
The dont_inherit argument, if non-zero, stops the compilation inheriting\n\
the effects of any future statements in effect in the code calling\n\
compile; if absent or zero these statements do influence the compilation,\n\
in addition to any features explicitly specified.");

static PyObject *
builtin_dir(PyObject *self, PyObject *args)
{
	PyObject *arg = NULL;

	if (!PyArg_UnpackTuple(args, "dir", 0, 1, &arg))
		return NULL;
	return PyObject_Dir(arg);
}

PyDoc_STRVAR(dir_doc,
"dir([object]) -> list of strings\n"
"\n"
"If called without an argument, return the names in the current scope.\n"
"Else, return an alphabetized list of names comprising (some of) the attributes\n"
"of the given object, and of attributes reachable from it.\n"
"If the object supplies a method named __dir__, it will be used; otherwise\n"
"the default dir() logic is used and returns:\n"
"  for a module object: the module's attributes.\n"
"  for a class object:  its attributes, and recursively the attributes\n"
"    of its bases.\n"
"  for any other object: its attributes, its class's attributes, and\n"
"    recursively the attributes of its class's base classes.");

static PyObject *
builtin_divmod(PyObject *self, PyObject *args)
{
	PyObject *v, *w;

	if (!PyArg_UnpackTuple(args, "divmod", 2, 2, &v, &w))
		return NULL;
	return PyNumber_Divmod(v, w);
}

PyDoc_STRVAR(divmod_doc,
"divmod(x, y) -> (div, mod)\n\
\n\
Return the tuple ((x-x%y)/y, x%y).  Invariant: div*y + mod == x.");


static PyObject *
builtin_eval(PyObject *self, PyObject *args)
{
	PyObject *cmd, *result, *tmp = NULL;
	PyObject *globals = Py_None, *locals = Py_None;
	char *str;
	PyCompilerFlags cf;

	if (!PyArg_UnpackTuple(args, "eval", 1, 3, &cmd, &globals, &locals))
		return NULL;
	if (locals != Py_None && !PyMapping_Check(locals)) {
		PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
		return NULL;
	}
	if (globals != Py_None && !PyDict_Check(globals)) {
		PyErr_SetString(PyExc_TypeError, PyMapping_Check(globals) ?
			"globals must be a real dict; try eval(expr, {}, mapping)"
			: "globals must be a dict");
		return NULL;
	}
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None)
			locals = PyEval_GetLocals();
	}
	else if (locals == Py_None)
		locals = globals;

	if (globals == NULL || locals == NULL) {
		PyErr_SetString(PyExc_TypeError, 
			"eval must be given globals and locals "
			"when called without a frame");
		return NULL;
	}

	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}

	if (PyCode_Check(cmd)) {
		if (PyCode_GetNumFree((PyCodeObject *)cmd) > 0) {
			PyErr_SetString(PyExc_TypeError,
		"code object passed to eval() may not contain free variables");
			return NULL;
		}
		return PyEval_EvalCode((PyCodeObject *) cmd, globals, locals);
	}

	str = source_as_string(cmd);
	if (str == NULL)
		return NULL;

	while (*str == ' ' || *str == '\t')
		str++;

	cf.cf_flags = PyCF_SOURCE_IS_UTF8;
	(void)PyEval_MergeCompilerFlags(&cf);
	result = PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
	Py_XDECREF(tmp);
	return result;
}

PyDoc_STRVAR(eval_doc,
"eval(source[, globals[, locals]]) -> value\n\
\n\
Evaluate the source in the context of globals and locals.\n\
The source may be a string representing a Python expression\n\
or a code object as returned by compile().\n\
The globals must be a dictionary and locals can be any mapping,\n\
defaulting to the current globals and locals.\n\
If only globals is given, locals defaults to it.\n");

static PyObject *
builtin_exec(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *prog, *globals = Py_None, *locals = Py_None;
	int plain = 0;

	if (!PyArg_ParseTuple(args, "O|OO:exec", &prog, &globals, &locals))
		return NULL;
	
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None) {
			locals = PyEval_GetLocals();
			plain = 1;
		}
		if (!globals || !locals) {
			PyErr_SetString(PyExc_SystemError,
					"globals and locals cannot be NULL");
			return NULL;
		}
	}
	else if (locals == Py_None)
		locals = globals;
	if (!PyString_Check(prog) &&
	    !PyUnicode_Check(prog) &&
	    !PyCode_Check(prog) &&
	    !PyFile_Check(prog)) {
		PyErr_Format(PyExc_TypeError,
			"exec() arg 1 must be a string, file, or code "
			"object, not %.100s", prog->ob_type->tp_name);
		return NULL;
	}
	if (!PyDict_Check(globals)) {
		PyErr_Format(PyExc_TypeError, "exec() arg 2 must be a dict, not %.100s",
			     globals->ob_type->tp_name);
		return NULL;
	}
	if (!PyMapping_Check(locals)) {
		PyErr_Format(PyExc_TypeError,
		    "arg 3 must be a mapping or None, not %.100s",
		    locals->ob_type->tp_name);
		return NULL;
	}
	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}

	if (PyCode_Check(prog)) {
		if (PyCode_GetNumFree((PyCodeObject *)prog) > 0) {
			PyErr_SetString(PyExc_TypeError,
				"code object passed to exec() may not "
				"contain free variables");
			return NULL;
		}
		v = PyEval_EvalCode((PyCodeObject *) prog, globals, locals);
	}
	else if (PyFile_Check(prog)) {
		FILE *fp = PyFile_AsFile(prog);
		char *name = PyString_AsString(PyFile_Name(prog));
		PyCompilerFlags cf;
		cf.cf_flags = 0;
		if (PyEval_MergeCompilerFlags(&cf))
			v = PyRun_FileFlags(fp, name, Py_file_input, globals,
					    locals, &cf);
		else
			v = PyRun_File(fp, name, Py_file_input, globals,
				       locals);
	}
	else {
		char *str = source_as_string(prog);
		PyCompilerFlags cf;
		if (str == NULL)
			return NULL;
		cf.cf_flags = PyCF_SOURCE_IS_UTF8;
		if (PyEval_MergeCompilerFlags(&cf))
			v = PyRun_StringFlags(str, Py_file_input, globals,
					      locals, &cf);
		else
			v = PyRun_String(str, Py_file_input, globals, locals);
	}
	if (v == NULL)
		return NULL;
	Py_DECREF(v);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(exec_doc,
"exec(object[, globals[, locals]])\n\
\n\
Read and execute code from a object, which can be a string, a code\n\
object or a file object.\n\
The globals and locals are dictionaries, defaulting to the current\n\
globals and locals.  If only globals is given, locals defaults to it.");


static PyObject *
builtin_execfile(PyObject *self, PyObject *args)
{
	char *filename;
	PyObject *globals = Py_None, *locals = Py_None;
	PyObject *res;
	FILE* fp = NULL;
	PyCompilerFlags cf;
	int exists;

	if (!PyArg_ParseTuple(args, "s|O!O:execfile",
			&filename,
			&PyDict_Type, &globals,
			&locals))
		return NULL;
	if (locals != Py_None && !PyMapping_Check(locals)) {
		PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
		return NULL;
	}
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None)
			locals = PyEval_GetLocals();
	}
	else if (locals == Py_None)
		locals = globals;
	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}

	exists = 0;
	/* Test for existence or directory. */
#if defined(PLAN9)
	{
		Dir *d;

		if ((d = dirstat(filename))!=nil) {
			if(d->mode & DMDIR)
				werrstr("is a directory");
			else
				exists = 1;
			free(d);
		}
	}
#elif defined(RISCOS)
	if (object_exists(filename)) {
		if (isdir(filename))
			errno = EISDIR;
		else
			exists = 1;
	}
#else	/* standard Posix */
	{
		struct stat s;
		if (stat(filename, &s) == 0) {
			if (S_ISDIR(s.st_mode))
#				if defined(PYOS_OS2) && defined(PYCC_VACPP)
					errno = EOS2ERR;
#				else
					errno = EISDIR;
#				endif
			else
				exists = 1;
		}
	}
#endif

        if (exists) {
		Py_BEGIN_ALLOW_THREADS
		fp = fopen(filename, "r" PY_STDIOTEXTMODE);
		Py_END_ALLOW_THREADS

		if (fp == NULL) {
			exists = 0;
		}
        }

	if (!exists) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
		return NULL;
	}
	cf.cf_flags = 0;
	if (PyEval_MergeCompilerFlags(&cf))
		res = PyRun_FileExFlags(fp, filename, Py_file_input, globals,
				   locals, 1, &cf);
	else
		res = PyRun_FileEx(fp, filename, Py_file_input, globals,
				   locals, 1);
	return res;
}

PyDoc_STRVAR(execfile_doc,
"execfile(filename[, globals[, locals]])\n\
\n\
Read and execute a Python script from a file.\n\
The globals and locals are dictionaries, defaulting to the current\n\
globals and locals.  If only globals is given, locals defaults to it.");


static PyObject *
builtin_getattr(PyObject *self, PyObject *args)
{
	PyObject *v, *result, *dflt = NULL;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "getattr", 2, 3, &v, &name, &dflt))
		return NULL;
	if (PyUnicode_Check(name)) {
		name = _PyUnicode_AsDefaultEncodedString(name, NULL);
		if (name == NULL)
			return NULL;
	}

	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"getattr(): attribute name must be string");
		return NULL;
	}
	result = PyObject_GetAttr(v, name);
	if (result == NULL && dflt != NULL &&
	    PyErr_ExceptionMatches(PyExc_AttributeError))
	{
		PyErr_Clear();
		Py_INCREF(dflt);
		result = dflt;
	}
	return result;
}

PyDoc_STRVAR(getattr_doc,
"getattr(object, name[, default]) -> value\n\
\n\
Get a named attribute from an object; getattr(x, 'y') is equivalent to x.y.\n\
When a default argument is given, it is returned when the attribute doesn't\n\
exist; without it, an exception is raised in that case.");


static PyObject *
builtin_globals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetGlobals();
	Py_XINCREF(d);
	return d;
}

PyDoc_STRVAR(globals_doc,
"globals() -> dictionary\n\
\n\
Return the dictionary containing the current scope's global variables.");


static PyObject *
builtin_hasattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "hasattr", 2, 2, &v, &name))
		return NULL;
	if (PyUnicode_Check(name)) {
		name = _PyUnicode_AsDefaultEncodedString(name, NULL);
		if (name == NULL)
			return NULL;
	}

	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"hasattr(): attribute name must be string");
		return NULL;
	}
	v = PyObject_GetAttr(v, name);
	if (v == NULL) {
		PyErr_Clear();
		Py_INCREF(Py_False);
		return Py_False;
	}
	Py_DECREF(v);
	Py_INCREF(Py_True);
	return Py_True;
}

PyDoc_STRVAR(hasattr_doc,
"hasattr(object, name) -> bool\n\
\n\
Return whether the object has an attribute with the given name.\n\
(This is done by calling getattr(object, name) and catching exceptions.)");


static PyObject *
builtin_id(PyObject *self, PyObject *v)
{
	return PyLong_FromVoidPtr(v);
}

PyDoc_STRVAR(id_doc,
"id(object) -> integer\n\
\n\
Return the identity of an object.  This is guaranteed to be unique among\n\
simultaneously existing objects.  (Hint: it's the object's memory address.)");


static PyObject *
builtin_map(PyObject *self, PyObject *args)
{
	typedef struct {
		PyObject *it;	/* the iterator object */
		int saw_StopIteration;  /* bool:  did the iterator end? */
	} sequence;

	PyObject *func, *result;
	sequence *seqs = NULL, *sqp;
	Py_ssize_t n, len;
	register int i, j;

	n = PyTuple_Size(args);
	if (n < 2) {
		PyErr_SetString(PyExc_TypeError,
				"map() requires at least two args");
		return NULL;
	}

	func = PyTuple_GetItem(args, 0);
	n--;

	if (func == Py_None && n == 1) {
		/* map(None, S) is the same as list(S). */
		return PySequence_List(PyTuple_GetItem(args, 1));
	}

	/* Get space for sequence descriptors.  Must NULL out the iterator
	 * pointers so that jumping to Fail_2 later doesn't see trash.
	 */
	if ((seqs = PyMem_NEW(sequence, n)) == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	for (i = 0; i < n; ++i) {
		seqs[i].it = (PyObject*)NULL;
		seqs[i].saw_StopIteration = 0;
	}

	/* Do a first pass to obtain iterators for the arguments, and set len
	 * to the largest of their lengths.
	 */
	len = 0;
	for (i = 0, sqp = seqs; i < n; ++i, ++sqp) {
		PyObject *curseq;
		Py_ssize_t curlen;

		/* Get iterator. */
		curseq = PyTuple_GetItem(args, i+1);
		sqp->it = PyObject_GetIter(curseq);
		if (sqp->it == NULL) {
			static char errmsg[] =
			    "argument %d to map() must support iteration";
			char errbuf[sizeof(errmsg) + 25];
			PyOS_snprintf(errbuf, sizeof(errbuf), errmsg, i+2);
			PyErr_SetString(PyExc_TypeError, errbuf);
			goto Fail_2;
		}

		/* Update len. */
		curlen = _PyObject_LengthHint(curseq);
		if (curlen < 0) {
			if (!PyErr_ExceptionMatches(PyExc_TypeError)  &&
			    !PyErr_ExceptionMatches(PyExc_AttributeError)) {
				goto Fail_2;
			}
			PyErr_Clear();
			curlen = 8;  /* arbitrary */
		}
		if (curlen > len)
			len = curlen;
	}

	/* Get space for the result list. */
	if ((result = (PyObject *) PyList_New(len)) == NULL)
		goto Fail_2;

	/* Iterate over the sequences until all have stopped. */
	for (i = 0; ; ++i) {
		PyObject *alist, *item=NULL, *value;
		int numactive = 0;

		if (func == Py_None && n == 1)
			alist = NULL;
		else if ((alist = PyTuple_New(n)) == NULL)
			goto Fail_1;

		for (j = 0, sqp = seqs; j < n; ++j, ++sqp) {
			if (sqp->saw_StopIteration) {
				Py_INCREF(Py_None);
				item = Py_None;
			}
			else {
				item = PyIter_Next(sqp->it);
				if (item)
					++numactive;
				else {
					if (PyErr_Occurred()) {
						Py_XDECREF(alist);
						goto Fail_1;
					}
					Py_INCREF(Py_None);
					item = Py_None;
					sqp->saw_StopIteration = 1;
				}
			}
			if (alist)
				PyTuple_SET_ITEM(alist, j, item);
			else
				break;
		}

		if (!alist)
			alist = item;

		if (numactive == 0) {
			Py_DECREF(alist);
			break;
		}

		if (func == Py_None)
			value = alist;
		else {
			value = PyEval_CallObject(func, alist);
			Py_DECREF(alist);
			if (value == NULL)
				goto Fail_1;
		}
		if (i >= len) {
			int status = PyList_Append(result, value);
			Py_DECREF(value);
			if (status < 0)
				goto Fail_1;
		}
		else if (PyList_SetItem(result, i, value) < 0)
		 	goto Fail_1;
	}

	if (i < len && PyList_SetSlice(result, i, len, NULL) < 0)
		goto Fail_1;

	goto Succeed;

Fail_1:
	Py_DECREF(result);
Fail_2:
	result = NULL;
Succeed:
	assert(seqs);
	for (i = 0; i < n; ++i)
		Py_XDECREF(seqs[i].it);
	PyMem_DEL(seqs);
	return result;
}

PyDoc_STRVAR(map_doc,
"map(function, sequence[, sequence, ...]) -> list\n\
\n\
Return a list of the results of applying the function to the items of\n\
the argument sequence(s).  If more than one sequence is given, the\n\
function is called with an argument list consisting of the corresponding\n\
item of each sequence, substituting None for missing values when not all\n\
sequences have the same length.  If the function is None, return a list of\n\
the items of the sequence (or a list of tuples if more than one sequence).");


static PyObject *
builtin_next(PyObject *self, PyObject *args)
{
	PyObject *it, *res;
	PyObject *def = NULL;

	if (!PyArg_UnpackTuple(args, "next", 1, 2, &it, &def))
		return NULL;
	if (!PyIter_Check(it)) {
		PyErr_Format(PyExc_TypeError,
			"%.200s object is not an iterator", it->ob_type->tp_name);
		return NULL;
	}
	
	res = (*it->ob_type->tp_iternext)(it);
	if (res == NULL) {
		if (def) {
			if (PyErr_Occurred() &&
			    !PyErr_ExceptionMatches(PyExc_StopIteration))
				return NULL;
			PyErr_Clear();
			Py_INCREF(def);
			return def;
		} else if (PyErr_Occurred()) {
			return NULL;
		} else {
			PyErr_SetNone(PyExc_StopIteration);
			return NULL;
		}
	}
	return res;
}

PyDoc_STRVAR(next_doc,
"next(iterator[, default])\n\
\n\
Return the next item from the iterator. If default is given and the iterator\n\
is exhausted, it is returned instead of raising StopIteration.");


static PyObject *
builtin_setattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;
	PyObject *value;

	if (!PyArg_UnpackTuple(args, "setattr", 3, 3, &v, &name, &value))
		return NULL;
	if (PyObject_SetAttr(v, name, value) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setattr_doc,
"setattr(object, name, value)\n\
\n\
Set a named attribute on an object; setattr(x, 'y', v) is equivalent to\n\
``x.y = v''.");


static PyObject *
builtin_delattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "delattr", 2, 2, &v, &name))
		return NULL;
	if (PyObject_SetAttr(v, name, (PyObject *)NULL) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(delattr_doc,
"delattr(object, name)\n\
\n\
Delete a named attribute on an object; delattr(x, 'y') is equivalent to\n\
``del x.y''.");


static PyObject *
builtin_hash(PyObject *self, PyObject *v)
{
	long x;

	x = PyObject_Hash(v);
	if (x == -1)
		return NULL;
	return PyInt_FromLong(x);
}

PyDoc_STRVAR(hash_doc,
"hash(object) -> integer\n\
\n\
Return a hash value for the object.  Two objects with the same value have\n\
the same hash value.  The reverse is not necessarily true, but likely.");


static PyObject *
builtin_hex(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;
	PyObject *res;

	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_hex == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "hex() argument can't be converted to hex");
		return NULL;
	}
	res = (*nb->nb_hex)(v);
	if (res && !PyString_Check(res) && !PyUnicode_Check(res)) {
		PyErr_Format(PyExc_TypeError,
			     "__hex__ returned non-string (type %.200s)",
			     res->ob_type->tp_name);
		Py_DECREF(res);
		return NULL;
	}
	return res;
}

PyDoc_STRVAR(hex_doc,
"hex(number) -> string\n\
\n\
Return the hexadecimal representation of an integer or long integer.");


static PyObject *
builtin_iter(PyObject *self, PyObject *args)
{
	PyObject *v, *w = NULL;

	if (!PyArg_UnpackTuple(args, "iter", 1, 2, &v, &w))
		return NULL;
	if (w == NULL)
		return PyObject_GetIter(v);
	if (!PyCallable_Check(v)) {
		PyErr_SetString(PyExc_TypeError,
				"iter(v, w): v must be callable");
		return NULL;
	}
	return PyCallIter_New(v, w);
}

PyDoc_STRVAR(iter_doc,
"iter(collection) -> iterator\n\
iter(callable, sentinel) -> iterator\n\
\n\
Get an iterator from an object.  In the first form, the argument must\n\
supply its own iterator, or be a sequence.\n\
In the second form, the callable is called until it returns the sentinel.");


static PyObject *
builtin_len(PyObject *self, PyObject *v)
{
	Py_ssize_t res;

	res = PyObject_Size(v);
	if (res < 0 && PyErr_Occurred())
		return NULL;
	return PyInt_FromSsize_t(res);
}

PyDoc_STRVAR(len_doc,
"len(object) -> integer\n\
\n\
Return the number of items of a sequence or mapping.");


static PyObject *
builtin_locals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetLocals();
	Py_XINCREF(d);
	return d;
}

PyDoc_STRVAR(locals_doc,
"locals() -> dictionary\n\
\n\
Update and return a dictionary containing the current scope's local variables.");


static PyObject *
min_max(PyObject *args, PyObject *kwds, int op)
{
	PyObject *v, *it, *item, *val, *maxitem, *maxval, *keyfunc=NULL;
	const char *name = op == Py_LT ? "min" : "max";

	if (PyTuple_Size(args) > 1)
		v = args;
	else if (!PyArg_UnpackTuple(args, (char *)name, 1, 1, &v))
		return NULL;

	if (kwds != NULL && PyDict_Check(kwds) && PyDict_Size(kwds)) {
		keyfunc = PyDict_GetItemString(kwds, "key");
		if (PyDict_Size(kwds)!=1  ||  keyfunc == NULL) {
			PyErr_Format(PyExc_TypeError,
				"%s() got an unexpected keyword argument", name);
			return NULL;
		}
	}

	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;

	maxitem = NULL; /* the result */
	maxval = NULL;  /* the value associated with the result */
	while (( item = PyIter_Next(it) )) {
		/* get the value from the key function */
		if (keyfunc != NULL) {
			val = PyObject_CallFunctionObjArgs(keyfunc, item, NULL);
			if (val == NULL)
				goto Fail_it_item;
		}
		/* no key function; the value is the item */
		else {
			val = item;
			Py_INCREF(val);
		}

		/* maximum value and item are unset; set them */
		if (maxval == NULL) {
			maxitem = item;
			maxval = val;
		}
		/* maximum value and item are set; update them as necessary */
		else {
			int cmp = PyObject_RichCompareBool(val, maxval, op);
			if (cmp < 0)
				goto Fail_it_item_and_val;
			else if (cmp > 0) {
				Py_DECREF(maxval);
				Py_DECREF(maxitem);
				maxval = val;
				maxitem = item;
			}
			else {
				Py_DECREF(item);
				Py_DECREF(val);
			}
		}
	}
	if (PyErr_Occurred())
		goto Fail_it;
	if (maxval == NULL) {
		PyErr_Format(PyExc_ValueError,
			     "%s() arg is an empty sequence", name);
		assert(maxitem == NULL);
	}
	else
		Py_DECREF(maxval);
	Py_DECREF(it);
	return maxitem;

Fail_it_item_and_val:
	Py_DECREF(val);
Fail_it_item:
	Py_DECREF(item);
Fail_it:
	Py_XDECREF(maxval);
	Py_XDECREF(maxitem);
	Py_DECREF(it);
	return NULL;
}

static PyObject *
builtin_min(PyObject *self, PyObject *args, PyObject *kwds)
{
	return min_max(args, kwds, Py_LT);
}

PyDoc_STRVAR(min_doc,
"min(iterable[, key=func]) -> value\n\
min(a, b, c, ...[, key=func]) -> value\n\
\n\
With a single iterable argument, return its smallest item.\n\
With two or more arguments, return the smallest argument.");


static PyObject *
builtin_max(PyObject *self, PyObject *args, PyObject *kwds)
{
	return min_max(args, kwds, Py_GT);
}

PyDoc_STRVAR(max_doc,
"max(iterable[, key=func]) -> value\n\
max(a, b, c, ...[, key=func]) -> value\n\
\n\
With a single iterable argument, return its largest item.\n\
With two or more arguments, return the largest argument.");


static PyObject *
builtin_oct(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;
	PyObject *res;

	if (v == NULL || (nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_oct == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "oct() argument can't be converted to oct");
		return NULL;
	}
	res = (*nb->nb_oct)(v);
	if (res && !PyString_Check(res) && !PyUnicode_Check(res)) {
		PyErr_Format(PyExc_TypeError,
			     "__oct__ returned non-string (type %.200s)",
			     res->ob_type->tp_name);
		Py_DECREF(res);
		return NULL;
	}
	return res;
}

PyDoc_STRVAR(oct_doc,
"oct(number) -> string\n\
\n\
Return the octal representation of an integer or long integer.");


static PyObject *
builtin_ord(PyObject *self, PyObject* obj)
{
	long ord;
	Py_ssize_t size;

	if (PyString_Check(obj)) {
		size = PyString_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)((unsigned char)*PyString_AS_STRING(obj));
			return PyInt_FromLong(ord);
		}
	}
	else if (PyUnicode_Check(obj)) {
		size = PyUnicode_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)*PyUnicode_AS_UNICODE(obj);
			return PyInt_FromLong(ord);
		}
	}
	else if (PyBytes_Check(obj)) {
		/* XXX Hopefully this is temporary */
		size = PyBytes_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)((unsigned char)*PyBytes_AS_STRING(obj));
			return PyInt_FromLong(ord);
		}
	}
	else {
		PyErr_Format(PyExc_TypeError,
			     "ord() expected string of length 1, but " \
			     "%.200s found", obj->ob_type->tp_name);
		return NULL;
	}

	PyErr_Format(PyExc_TypeError,
		     "ord() expected a character, "
		     "but string of length %zd found",
		     size);
	return NULL;
}

PyDoc_STRVAR(ord_doc,
"ord(c) -> integer\n\
\n\
Return the integer ordinal of a one-character string.");


static PyObject *
builtin_pow(PyObject *self, PyObject *args)
{
	PyObject *v, *w, *z = Py_None;

	if (!PyArg_UnpackTuple(args, "pow", 2, 3, &v, &w, &z))
		return NULL;
	return PyNumber_Power(v, w, z);
}

PyDoc_STRVAR(pow_doc,
"pow(x, y[, z]) -> number\n\
\n\
With two arguments, equivalent to x**y.  With three arguments,\n\
equivalent to (x**y) % z, but may be more efficient (e.g. for longs).");



static PyObject *
builtin_print(PyObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"sep", "end", "file", 0};
	static PyObject *dummy_args;
	PyObject *sep = NULL, *end = NULL, *file = NULL;
	int i, err;

	if (dummy_args == NULL) {
		if (!(dummy_args = PyTuple_New(0)))
			return NULL;
	}
	if (!PyArg_ParseTupleAndKeywords(dummy_args, kwds, "|OOO:print",
					 kwlist, &sep, &end, &file))
                return NULL;
	if (file == NULL || file == Py_None)
		file = PySys_GetObject("stdout");

	if (sep && sep != Py_None && !PyString_Check(sep) &&
	    !PyUnicode_Check(sep)) {
		PyErr_Format(PyExc_TypeError,
			     "sep must be None, str or unicode, not %.200s",
			     sep->ob_type->tp_name);
		return NULL;
	}
	if (end && end != Py_None && !PyString_Check(end) &&
	    !PyUnicode_Check(end)) {
		PyErr_Format(PyExc_TypeError,
			     "end must be None, str or unicode, not %.200s",
			     end->ob_type->tp_name);
		return NULL;
	}

	for (i = 0; i < PyTuple_Size(args); i++) {
		if (i > 0) {
			if (sep == NULL || sep == Py_None)
				err = PyFile_WriteString(" ", file);
			else
				err = PyFile_WriteObject(sep, file,
							 Py_PRINT_RAW);
			if (err)
				return NULL;
		}
		err = PyFile_WriteObject(PyTuple_GetItem(args, i), file,
					 Py_PRINT_RAW);
		if (err)
			return NULL;
	}

	if (end == NULL || end == Py_None)
		err = PyFile_WriteString("\n", file);
	else
		err = PyFile_WriteObject(end, file, Py_PRINT_RAW);
	if (err)
		return NULL;

	Py_RETURN_NONE;
}

PyDoc_STRVAR(print_doc,
"print(value, ..., file=None, sep=' ', end='\\n')\n\
\n\
Prints the values to a stream, or to sys.stdout by default.\n\
Optional keyword arguments:\n\
file: a file-like object (stream); defaults to the current sys.stdout.\n\
sep:  string inserted between values, default a space.\n\
end:  string appended after the last value, default a newline.");


static PyObject *
builtin_input(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *fin = PySys_GetObject("stdin");
	PyObject *fout = PySys_GetObject("stdout");

	if (!PyArg_UnpackTuple(args, "input", 0, 1, &v))
		return NULL;

	if (fin == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "input: lost sys.stdin");
		return NULL;
	}
	if (fout == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "input: lost sys.stdout");
		return NULL;
	}
	if (PyFile_AsFile(fin) && PyFile_AsFile(fout)
            && isatty(fileno(PyFile_AsFile(fin)))
            && isatty(fileno(PyFile_AsFile(fout)))) {
		PyObject *po;
		char *prompt;
		char *s;
		PyObject *result;
		if (v != NULL) {
			po = PyObject_Str(v);
			if (po == NULL)
				return NULL;
			prompt = PyString_AsString(po);
			if (prompt == NULL)
				return NULL;
		}
		else {
			po = NULL;
			prompt = "";
		}
		s = PyOS_Readline(PyFile_AsFile(fin), PyFile_AsFile(fout),
                                  prompt);
		Py_XDECREF(po);
		if (s == NULL) {
			if (!PyErr_Occurred())
				PyErr_SetNone(PyExc_KeyboardInterrupt);
			return NULL;
		}
		if (*s == '\0') {
			PyErr_SetNone(PyExc_EOFError);
			result = NULL;
		}
		else { /* strip trailing '\n' */
			size_t len = strlen(s);
			if (len > PY_SSIZE_T_MAX) {
				PyErr_SetString(PyExc_OverflowError,
						"input: input too long");
				result = NULL;
			}
			else {
				result = PyString_FromStringAndSize(s, len-1);
			}
		}
		PyMem_FREE(s);
		return result;
	}
	if (v != NULL) {
		if (PyFile_WriteObject(v, fout, Py_PRINT_RAW) != 0)
			return NULL;
	}
	return PyFile_GetLine(fin, -1);
}

PyDoc_STRVAR(input_doc,
"input([prompt]) -> string\n\
\n\
Read a string from standard input.  The trailing newline is stripped.\n\
If the user hits EOF (Unix: Ctl-D, Windows: Ctl-Z+Return), raise EOFError.\n\
On Unix, GNU readline is used if enabled.  The prompt string, if given,\n\
is printed without a trailing newline before reading.");

static PyObject *
builtin_reload(PyObject *self, PyObject *v)
{
	return PyImport_ReloadModule(v);
}

PyDoc_STRVAR(reload_doc,
"reload(module) -> module\n\
\n\
Reload the module.  The module must have been successfully imported before.");


static PyObject *
builtin_repr(PyObject *self, PyObject *v)
{
	return PyObject_Repr(v);
}

PyDoc_STRVAR(repr_doc,
"repr(object) -> string\n\
\n\
Return the canonical string representation of the object.\n\
For most object types, eval(repr(object)) == object.");


static PyObject *
builtin_round(PyObject *self, PyObject *args, PyObject *kwds)
{
	double number;
	double f;
	int ndigits = 0;
	int i;
	static char *kwlist[] = {"number", "ndigits", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "d|i:round",
                kwlist, &number, &ndigits))
                return NULL;
	f = 1.0;
	i = abs(ndigits);
	while  (--i >= 0)
		f = f*10.0;
	if (ndigits < 0)
		number /= f;
	else
		number *= f;
	if (number >= 0.0)
		number = floor(number + 0.5);
	else
		number = ceil(number - 0.5);
	if (ndigits < 0)
		number *= f;
	else
		number /= f;
	return PyFloat_FromDouble(number);
}

PyDoc_STRVAR(round_doc,
"round(number[, ndigits]) -> floating point number\n\
\n\
Round a number to a given precision in decimal digits (default 0 digits).\n\
This always returns a floating point number.  Precision may be negative.");

static PyObject *
builtin_sorted(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *newlist, *v, *seq, *compare=NULL, *keyfunc=NULL, *newargs;
	PyObject *callable;
	static char *kwlist[] = {"iterable", "cmp", "key", "reverse", 0};
	int reverse;

	/* args 1-4 should match listsort in Objects/listobject.c */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOi:sorted",
		kwlist, &seq, &compare, &keyfunc, &reverse))
		return NULL;

	newlist = PySequence_List(seq);
	if (newlist == NULL)
		return NULL;

	callable = PyObject_GetAttrString(newlist, "sort");
	if (callable == NULL) {
		Py_DECREF(newlist);
		return NULL;
	}

	newargs = PyTuple_GetSlice(args, 1, 4);
	if (newargs == NULL) {
		Py_DECREF(newlist);
		Py_DECREF(callable);
		return NULL;
	}

	v = PyObject_Call(callable, newargs, kwds);
	Py_DECREF(newargs);
	Py_DECREF(callable);
	if (v == NULL) {
		Py_DECREF(newlist);
		return NULL;
	}
	Py_DECREF(v);
	return newlist;
}

PyDoc_STRVAR(sorted_doc,
"sorted(iterable, cmp=None, key=None, reverse=False) --> new sorted list");

static PyObject *
builtin_vars(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *d;

	if (!PyArg_UnpackTuple(args, "vars", 0, 1, &v))
		return NULL;
	if (v == NULL) {
		d = PyEval_GetLocals();
		if (d == NULL) {
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_SystemError,
						"vars(): no locals!?");
		}
		else
			Py_INCREF(d);
	}
	else {
		d = PyObject_GetAttrString(v, "__dict__");
		if (d == NULL) {
			PyErr_SetString(PyExc_TypeError,
			    "vars() argument must have __dict__ attribute");
			return NULL;
		}
	}
	return d;
}

PyDoc_STRVAR(vars_doc,
"vars([object]) -> dictionary\n\
\n\
Without arguments, equivalent to locals().\n\
With an argument, equivalent to object.__dict__.");


static PyObject*
builtin_sum(PyObject *self, PyObject *args)
{
	PyObject *seq;
	PyObject *result = NULL;
	PyObject *temp, *item, *iter;

	if (!PyArg_UnpackTuple(args, "sum", 1, 2, &seq, &result))
		return NULL;

	iter = PyObject_GetIter(seq);
	if (iter == NULL)
		return NULL;

	if (result == NULL) {
		result = PyInt_FromLong(0);
		if (result == NULL) {
			Py_DECREF(iter);
			return NULL;
		}
	} else {
		/* reject string values for 'start' parameter */
		if (PyObject_TypeCheck(result, &PyBaseString_Type)) {
			PyErr_SetString(PyExc_TypeError,
				"sum() can't sum strings [use ''.join(seq) instead]");
			Py_DECREF(iter);
			return NULL;
		}
		Py_INCREF(result);
	}

	for(;;) {
		item = PyIter_Next(iter);
		if (item == NULL) {
			/* error, or end-of-sequence */
			if (PyErr_Occurred()) {
				Py_DECREF(result);
				result = NULL;
			}
			break;
		}
		temp = PyNumber_Add(result, item);
		Py_DECREF(result);
		Py_DECREF(item);
		result = temp;
		if (result == NULL)
			break;
	}
	Py_DECREF(iter);
	return result;
}

PyDoc_STRVAR(sum_doc,
"sum(sequence[, start]) -> value\n\
\n\
Returns the sum of a sequence of numbers (NOT strings) plus the value\n\
of parameter 'start' (which defaults to 0).  When the sequence is\n\
empty, returns start.");


static PyObject *
builtin_isinstance(PyObject *self, PyObject *args)
{
	PyObject *inst;
	PyObject *cls;
	int retval;

	if (!PyArg_UnpackTuple(args, "isinstance", 2, 2, &inst, &cls))
		return NULL;

	retval = PyObject_IsInstance(inst, cls);
	if (retval < 0)
		return NULL;
	return PyBool_FromLong(retval);
}

PyDoc_STRVAR(isinstance_doc,
"isinstance(object, class-or-type-or-tuple) -> bool\n\
\n\
Return whether an object is an instance of a class or of a subclass thereof.\n\
With a type as second argument, return whether that is the object's type.\n\
The form using a tuple, isinstance(x, (A, B, ...)), is a shortcut for\n\
isinstance(x, A) or isinstance(x, B) or ... (etc.).");


static PyObject *
builtin_issubclass(PyObject *self, PyObject *args)
{
	PyObject *derived;
	PyObject *cls;
	int retval;

	if (!PyArg_UnpackTuple(args, "issubclass", 2, 2, &derived, &cls))
		return NULL;

	retval = PyObject_IsSubclass(derived, cls);
	if (retval < 0)
		return NULL;
	return PyBool_FromLong(retval);
}

PyDoc_STRVAR(issubclass_doc,
"issubclass(C, B) -> bool\n\
\n\
Return whether class C is a subclass (i.e., a derived class) of class B.\n\
When using a tuple as the second argument issubclass(X, (A, B, ...)),\n\
is a shortcut for issubclass(X, A) or issubclass(X, B) or ... (etc.).");


static PyObject*
builtin_zip(PyObject *self, PyObject *args)
{
	/* args must be a tuple */
	assert(PyTuple_Check(args));

	return _PyZip_CreateIter(args);
}


PyDoc_STRVAR(zip_doc,
"zip(it1 [, it2 [...]]) -> iter([(it1[0], it2[0] ...), ...])\n\
\n\
Return an iterator yielding tuples, where each tuple contains the\n\
corresponding element from each of the argument iterables.\n\
The returned iterator ends when the shortest argument iterable is exhausted.\n\
NOTE: This is implemented using itertools.izip().");


static PyMethodDef builtin_methods[] = {
 	{"__build_class__", (PyCFunction)builtin___build_class__,
         METH_VARARGS | METH_KEYWORDS, build_class_doc},
 	{"__import__",	(PyCFunction)builtin___import__, METH_VARARGS | METH_KEYWORDS, import_doc},
 	{"abs",		builtin_abs,        METH_O, abs_doc},
 	{"all",		builtin_all,        METH_O, all_doc},
 	{"any",		builtin_any,        METH_O, any_doc},
 	{"chr",		builtin_unichr,     METH_VARARGS, unichr_doc},
 	{"chr8",	builtin_chr,        METH_VARARGS, chr_doc},
 	{"cmp",		builtin_cmp,        METH_VARARGS, cmp_doc},
 	{"compile",	(PyCFunction)builtin_compile,    METH_VARARGS | METH_KEYWORDS, compile_doc},
 	{"delattr",	builtin_delattr,    METH_VARARGS, delattr_doc},
 	{"dir",		builtin_dir,        METH_VARARGS, dir_doc},
 	{"divmod",	builtin_divmod,     METH_VARARGS, divmod_doc},
 	{"eval",	builtin_eval,       METH_VARARGS, eval_doc},
	{"exec",        builtin_exec,       METH_VARARGS, exec_doc},
 	{"execfile",	builtin_execfile,   METH_VARARGS, execfile_doc},
 	{"filter",	builtin_filter,     METH_VARARGS, filter_doc},
 	{"getattr",	builtin_getattr,    METH_VARARGS, getattr_doc},
 	{"globals",	(PyCFunction)builtin_globals,    METH_NOARGS, globals_doc},
 	{"hasattr",	builtin_hasattr,    METH_VARARGS, hasattr_doc},
 	{"hash",	builtin_hash,       METH_O, hash_doc},
 	{"hex",		builtin_hex,        METH_O, hex_doc},
 	{"id",		builtin_id,         METH_O, id_doc},
 	{"input",	builtin_input,      METH_VARARGS, input_doc},
 	{"isinstance",  builtin_isinstance, METH_VARARGS, isinstance_doc},
 	{"issubclass",  builtin_issubclass, METH_VARARGS, issubclass_doc},
 	{"iter",	builtin_iter,       METH_VARARGS, iter_doc},
 	{"len",		builtin_len,        METH_O, len_doc},
 	{"locals",	(PyCFunction)builtin_locals,     METH_NOARGS, locals_doc},
 	{"map",		builtin_map,        METH_VARARGS, map_doc},
 	{"max",		(PyCFunction)builtin_max,        METH_VARARGS | METH_KEYWORDS, max_doc},
 	{"min",		(PyCFunction)builtin_min,        METH_VARARGS | METH_KEYWORDS, min_doc},
	{"next",	(PyCFunction)builtin_next,       METH_VARARGS, next_doc},
 	{"oct",		builtin_oct,        METH_O, oct_doc},
 	{"ord",		builtin_ord,        METH_O, ord_doc},
 	{"pow",		builtin_pow,        METH_VARARGS, pow_doc},
 	{"print",	(PyCFunction)builtin_print,      METH_VARARGS | METH_KEYWORDS, print_doc},
 	{"reload",	builtin_reload,     METH_O, reload_doc},
 	{"repr",	builtin_repr,       METH_O, repr_doc},
 	{"round",	(PyCFunction)builtin_round,      METH_VARARGS | METH_KEYWORDS, round_doc},
 	{"setattr",	builtin_setattr,    METH_VARARGS, setattr_doc},
 	{"sorted",	(PyCFunction)builtin_sorted,     METH_VARARGS | METH_KEYWORDS, sorted_doc},
 	{"sum",		builtin_sum,        METH_VARARGS, sum_doc},
 	{"vars",	builtin_vars,       METH_VARARGS, vars_doc},
  	{"zip",         builtin_zip,        METH_VARARGS, zip_doc},
	{NULL,		NULL},
};

PyDoc_STRVAR(builtin_doc,
"Built-in functions, exceptions, and other objects.\n\
\n\
Noteworthy: None is the `nil' object; Ellipsis represents `...' in slices.");

PyObject *
_PyBuiltin_Init(void)
{
	PyObject *mod, *dict, *debug;
	mod = Py_InitModule4("__builtin__", builtin_methods,
			     builtin_doc, (PyObject *)NULL,
			     PYTHON_API_VERSION);
	if (mod == NULL)
		return NULL;
	dict = PyModule_GetDict(mod);

#ifdef Py_TRACE_REFS
	/* __builtin__ exposes a number of statically allocated objects
	 * that, before this code was added in 2.3, never showed up in
	 * the list of "all objects" maintained by Py_TRACE_REFS.  As a
	 * result, programs leaking references to None and False (etc)
	 * couldn't be diagnosed by examining sys.getobjects(0).
	 */
#define ADD_TO_ALL(OBJECT) _Py_AddToAllObjects((PyObject *)(OBJECT), 0)
#else
#define ADD_TO_ALL(OBJECT) (void)0
#endif

#define SETBUILTIN(NAME, OBJECT) \
	if (PyDict_SetItemString(dict, NAME, (PyObject *)OBJECT) < 0)	\
		return NULL;						\
	ADD_TO_ALL(OBJECT)

	SETBUILTIN("None",		Py_None);
	SETBUILTIN("Ellipsis",		Py_Ellipsis);
	SETBUILTIN("NotImplemented",	Py_NotImplemented);
	SETBUILTIN("False",		Py_False);
	SETBUILTIN("True",		Py_True);
	SETBUILTIN("basestring",	&PyBaseString_Type);
	SETBUILTIN("bool",		&PyBool_Type);
	SETBUILTIN("buffer",		&PyBuffer_Type);
	SETBUILTIN("bytes",		&PyBytes_Type);
	SETBUILTIN("classmethod",	&PyClassMethod_Type);
#ifndef WITHOUT_COMPLEX
	SETBUILTIN("complex",		&PyComplex_Type);
#endif
	SETBUILTIN("dict",		&PyDict_Type);
 	SETBUILTIN("enumerate",		&PyEnum_Type);
	SETBUILTIN("float",		&PyFloat_Type);
	SETBUILTIN("frozenset",		&PyFrozenSet_Type);
	SETBUILTIN("property",		&PyProperty_Type);
	SETBUILTIN("int",		&PyLong_Type);
	SETBUILTIN("list",		&PyList_Type);
	SETBUILTIN("object",		&PyBaseObject_Type);
	SETBUILTIN("range",		&PyRange_Type);
	SETBUILTIN("reversed",		&PyReversed_Type);
	SETBUILTIN("set",		&PySet_Type);
	SETBUILTIN("slice",		&PySlice_Type);
	SETBUILTIN("staticmethod",	&PyStaticMethod_Type);
	SETBUILTIN("str",		&PyUnicode_Type);
	SETBUILTIN("str8",		&PyString_Type);
	SETBUILTIN("super",		&PySuper_Type);
	SETBUILTIN("tuple",		&PyTuple_Type);
	SETBUILTIN("type",		&PyType_Type);
	debug = PyBool_FromLong(Py_OptimizeFlag == 0);
	if (PyDict_SetItemString(dict, "__debug__", debug) < 0) {
		Py_XDECREF(debug);
		return NULL;
	}
	Py_XDECREF(debug);

	return mod;
#undef ADD_TO_ALL
#undef SETBUILTIN
}

/* Helper for filter(): filter a tuple through a function */

static PyObject *
filtertuple(PyObject *func, PyObject *tuple)
{
	PyObject *result;
	Py_ssize_t i, j;
	Py_ssize_t len = PyTuple_Size(tuple);

	if (len == 0) {
		if (PyTuple_CheckExact(tuple))
			Py_INCREF(tuple);
		else
			tuple = PyTuple_New(0);
		return tuple;
	}

	if ((result = PyTuple_New(len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *good;
		int ok;

		if (tuple->ob_type->tp_as_sequence &&
		    tuple->ob_type->tp_as_sequence->sq_item) {
			item = tuple->ob_type->tp_as_sequence->sq_item(tuple, i);
			if (item == NULL)
				goto Fail_1;
		} else {
			PyErr_SetString(PyExc_TypeError, "filter(): unsubscriptable tuple");
			goto Fail_1;
		}
		if (func == Py_None) {
			Py_INCREF(item);
			good = item;
		}
		else {
			PyObject *arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
		}
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
		if (ok) {
			if (PyTuple_SetItem(result, j++, item) < 0)
				goto Fail_1;
		}
		else
			Py_DECREF(item);
	}

	if (_PyTuple_Resize(&result, j) < 0)
		return NULL;

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}


/* Helper for filter(): filter a string through a function */

static PyObject *
filterstring(PyObject *func, PyObject *strobj)
{
	PyObject *result;
	Py_ssize_t i, j;
	Py_ssize_t len = PyString_Size(strobj);
	Py_ssize_t outlen = len;

	if (func == Py_None) {
		/* If it's a real string we can return the original,
		 * as no character is ever false and __getitem__
		 * does return this character. If it's a subclass
		 * we must go through the __getitem__ loop */
		if (PyString_CheckExact(strobj)) {
			Py_INCREF(strobj);
			return strobj;
		}
	}
	if ((result = PyString_FromStringAndSize(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		if (func==Py_None) {
			ok = 1;
		} else {
			PyObject *arg, *good;
			arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			Py_ssize_t reslen;
			if (!PyString_Check(item)) {
				PyErr_SetString(PyExc_TypeError, "can't filter str to str:"
					" __getitem__ returned different type");
				Py_DECREF(item);
				goto Fail_1;
			}
			reslen = PyString_GET_SIZE(item);
			if (reslen == 1) {
				PyString_AS_STRING(result)[j++] =
					PyString_AS_STRING(item)[0];
			} else {
				/* do we need more space? */
				Py_ssize_t need = j + reslen + len-i-1;
				if (need > outlen) {
					/* overallocate, to avoid reallocations */
					if (need<2*outlen)
						need = 2*outlen;
					if (_PyString_Resize(&result, need)) {
						Py_DECREF(item);
						return NULL;
					}
					outlen = need;
				}
				memcpy(
					PyString_AS_STRING(result) + j,
					PyString_AS_STRING(item),
					reslen
				);
				j += reslen;
			}
		}
		Py_DECREF(item);
	}

	if (j < outlen)
		_PyString_Resize(&result, j);

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}

/* Helper for filter(): filter a Unicode object through a function */

static PyObject *
filterunicode(PyObject *func, PyObject *strobj)
{
	PyObject *result;
	register Py_ssize_t i, j;
	Py_ssize_t len = PyUnicode_GetSize(strobj);
	Py_ssize_t outlen = len;

	if (func == Py_None) {
		/* If it's a real string we can return the original,
		 * as no character is ever false and __getitem__
		 * does return this character. If it's a subclass
		 * we must go through the __getitem__ loop */
		if (PyUnicode_CheckExact(strobj)) {
			Py_INCREF(strobj);
			return strobj;
		}
	}
	if ((result = PyUnicode_FromUnicode(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *arg, *good;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		if (func == Py_None) {
			ok = 1;
		} else {
			arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			Py_ssize_t reslen;
			if (!PyUnicode_Check(item)) {
				PyErr_SetString(PyExc_TypeError,
				"can't filter unicode to unicode:"
				" __getitem__ returned different type");
				Py_DECREF(item);
				goto Fail_1;
			}
			reslen = PyUnicode_GET_SIZE(item);
			if (reslen == 1)
				PyUnicode_AS_UNICODE(result)[j++] =
					PyUnicode_AS_UNICODE(item)[0];
			else {
				/* do we need more space? */
				Py_ssize_t need = j + reslen + len - i - 1;
				if (need > outlen) {
					/* overallocate,
					   to avoid reallocations */
					if (need < 2 * outlen)
						need = 2 * outlen;
					if (PyUnicode_Resize(
						&result, need) < 0) {
						Py_DECREF(item);
						goto Fail_1;
					}
					outlen = need;
				}
				memcpy(PyUnicode_AS_UNICODE(result) + j,
				       PyUnicode_AS_UNICODE(item),
				       reslen*sizeof(Py_UNICODE));
				j += reslen;
			}
		}
		Py_DECREF(item);
	}

	if (j < outlen)
		PyUnicode_Resize(&result, j);

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}
