/* Author: Daniel Stutzbach */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h> /* For offsetof */

/*
 * Known likely problems:
 *
 * - Files larger then 2**32-1
 * - Files with unicode filenames
 * - Passing numbers greater than 2**32-1 when an integer is expected
 * - Making it work on Windows and other oddball platforms
 *
 * To Do:
 *
 * - autoconfify header file inclusion
 */

#ifdef MS_WINDOWS
/* can simulate truncate with Win32 API functions; see file_truncate */
#define HAVE_FTRUNCATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef struct {
	PyObject_HEAD
	int fd;
	int readable;
	int writable;
	int seekable; /* -1 means unknown */
	PyObject *weakreflist;
} PyFileIOObject;

PyTypeObject PyFileIO_Type;

#define PyFileIO_Check(op) (PyObject_TypeCheck((op), &PyFileIO_Type))

/* Note: if this function is changed so that it can return a true value,
 * then we need a separate function for __exit__
 */
static PyObject *
fileio_close(PyFileIOObject *self)
{
	if (self->fd >= 0) {
		int fd = self->fd;
		self->fd = -1;
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		close(fd);
		Py_END_ALLOW_THREADS
		if (errno < 0) {
			PyErr_SetFromErrno(PyExc_IOError);
			return NULL;
		}
	}

	Py_RETURN_NONE;
}

static PyObject *
fileio_new(PyTypeObject *type, PyObject *args, PyObject *kews)
{
	PyFileIOObject *self;

	assert(type != NULL && type->tp_alloc != NULL);

	self = (PyFileIOObject *) type->tp_alloc(type, 0);
	if (self != NULL) {
		self->fd = -1;
		self->weakreflist = NULL;
	}

	return (PyObject *) self;
}

/* On Unix, open will succeed for directories.
   In Python, there should be no file objects referring to
   directories, so we need a check.  */

static int
dircheck(PyFileIOObject* self)
{
#if defined(HAVE_FSTAT) && defined(S_IFDIR) && defined(EISDIR)
	struct stat buf;
	if (self->fd < 0)
		return 0;
	if (fstat(self->fd, &buf) == 0 && S_ISDIR(buf.st_mode)) {
#ifdef HAVE_STRERROR
		char *msg = strerror(EISDIR);
#else
		char *msg = "Is a directory";
#endif
		PyObject *exc;
		PyObject *closeresult = fileio_close(self);
		Py_DECREF(closeresult);

		exc = PyObject_CallFunction(PyExc_IOError, "(is)",
					    EISDIR, msg);
		PyErr_SetObject(PyExc_IOError, exc);
		Py_XDECREF(exc);
		return -1;
	}
#endif
	return 0;
}


static int
fileio_init(PyObject *oself, PyObject *args, PyObject *kwds)
{
	PyFileIOObject *self = (PyFileIOObject *) oself;
	static char *kwlist[] = {"file", "mode", NULL};
	char *name = NULL;
	char *mode = "r";
	char *s;
	int wideargument = 0;
	int ret = 0;
	int rwa = 0, plus = 0, append = 0;
	int flags = 0;
	int fd = -1;

	assert(PyFileIO_Check(oself));
	if (self->fd >= 0)
	{
		/* Have to close the existing file first. */
		PyObject *closeresult = fileio_close(self);
		if (closeresult == NULL)
			return -1;
		Py_DECREF(closeresult);
	}

	if (PyArg_ParseTupleAndKeywords(args, kwds, "i|s:fileio",
					kwlist, &fd, &mode)) {
		if (fd < 0) {
			PyErr_SetString(PyExc_ValueError,
					"Negative filedescriptor");
			return -1;
		}
	}
	else {
		PyErr_Clear();

#ifdef Py_WIN_WIDE_FILENAMES
	    if (GetVersion() < 0x80000000) {
		/* On NT, so wide API available */
		PyObject *po;
		if (PyArg_ParseTupleAndKeywords(args, kwds, "U|s:fileio",
						kwlist, &po, &mode)) {
			wideargument = 1;
		} else {
			/* Drop the argument parsing error as narrow
			   strings are also valid. */
			PyErr_Clear();
		}

		PyErr_SetString(PyExc_NotImplementedError,
			"Windows wide filenames are not yet supported");
		goto error;
	    }
#endif

	    if (!wideargument) {
		if (!PyArg_ParseTupleAndKeywords(args, kwds, "et|s:fileio",
						 kwlist,
						 Py_FileSystemDefaultEncoding,
						 &name, &mode))
			goto error;
	    }
	}

	self->readable = self->writable = 0;
        self->seekable = -1;
	s = mode;
	while (*s) {
		switch (*s++) {
		case 'r':
			if (rwa) {
			bad_mode:
				PyErr_SetString(PyExc_ValueError,
						"Must have exactly one of read/write/append mode");
				goto error;
			}
			rwa = 1;
			self->readable = 1;
			break;
		case 'w':
			if (rwa)
				goto bad_mode;
			rwa = 1;
			self->writable = 1;
			flags |= O_CREAT | O_TRUNC;
			break;
		case 'a':
			if (rwa)
				goto bad_mode;
			rwa = 1;
			self->writable = 1;
			flags |= O_CREAT;
			append = 1;
			break;
		case '+':
			if (plus)
				goto bad_mode;
			self->readable = self->writable = 1;
			plus = 1;
			break;
		default:
			PyErr_Format(PyExc_ValueError,
				     "invalid mode: %.200s", mode);
			goto error;
		}
	}

	if (!rwa)
		goto bad_mode;

	if (self->readable && self->writable)
		flags |= O_RDWR;
	else if (self->readable)
		flags |= O_RDONLY;
	else
		flags |= O_WRONLY;

#ifdef O_BINARY
	flags |= O_BINARY;
#endif

	if (fd >= 0) {
		self->fd = fd;
	}
	else {
		Py_BEGIN_ALLOW_THREADS
		errno = 0;
		self->fd = open(name, flags, 0666);
		Py_END_ALLOW_THREADS
		if (self->fd < 0 || dircheck(self) < 0) {
			PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
			goto error;
		}
	}

	goto done;

 error:
	ret = -1;

 done:
	PyMem_Free(name);
	return ret;
}

static void
fileio_dealloc(PyFileIOObject *self)
{
	if (self->weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) self);

	if (self->fd >= 0) {
		PyObject *closeresult = fileio_close(self);
		if (closeresult == NULL) {
#ifdef HAVE_STRERROR
			PySys_WriteStderr("close failed: [Errno %d] %s\n",
                                          errno, strerror(errno));
#else
			PySys_WriteStderr("close failed: [Errno %d]\n", errno);
#endif
		} else
			Py_DECREF(closeresult);
	}

	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *
err_closed(void)
{
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
	return NULL;
}

static PyObject *
err_mode(char *action)
{
	PyErr_Format(PyExc_ValueError, "File not open for %s", action);
	return NULL;
}

static PyObject *
fileio_fileno(PyFileIOObject *self)
{
	if (self->fd < 0)
		return err_closed();
	return PyInt_FromLong((long) self->fd);
}

static PyObject *
fileio_readable(PyFileIOObject *self)
{
	if (self->fd < 0)
		return err_closed();
	return PyInt_FromLong((long) self->readable);
}

static PyObject *
fileio_writable(PyFileIOObject *self)
{
	if (self->fd < 0)
		return err_closed();
	return PyInt_FromLong((long) self->writable);
}

static PyObject *
fileio_seekable(PyFileIOObject *self)
{
	if (self->fd < 0)
		return err_closed();
	if (self->seekable < 0) {
		int ret;
		Py_BEGIN_ALLOW_THREADS
		ret = lseek(self->fd, 0, SEEK_CUR);
		Py_END_ALLOW_THREADS
		if (ret < 0)
			self->seekable = 0;
		else
			self->seekable = 1;
	}
	return PyInt_FromLong((long) self->seekable);
}

static PyObject *
fileio_readinto(PyFileIOObject *self, PyObject *args)
{
	char *ptr;
	Py_ssize_t n;

	if (self->fd < 0)
		return err_closed();
	if (!self->readable)
		return err_mode("reading");

	if (!PyArg_ParseTuple(args, "w#", &ptr, &n))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	n = read(self->fd, ptr, n);
	Py_END_ALLOW_THREADS
	if (n < 0) {
		if (errno == EAGAIN)
			Py_RETURN_NONE;
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	return PyInt_FromLong(n);
}

static PyObject *
fileio_read(PyFileIOObject *self, PyObject *args)
{
	char *ptr;
	Py_ssize_t n, size;
	PyObject *bytes;

	if (self->fd < 0)
		return err_closed();
	if (!self->readable)
		return err_mode("reading");

	if (!PyArg_ParseTuple(args, "i", &size))
		return NULL;

	bytes = PyBytes_FromStringAndSize(NULL, size);
	if (bytes == NULL)
		return NULL;
	ptr = PyBytes_AsString(bytes);

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	n = read(self->fd, ptr, size);
	Py_END_ALLOW_THREADS

	if (n < 0) {
		if (errno == EAGAIN)
			Py_RETURN_NONE;
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	if (n != size) {
		if (PyBytes_Resize(bytes, n) < 0) {
			Py_DECREF(bytes);
			return NULL;
		}
	}

	return (PyObject *) bytes;
}

static PyObject *
fileio_write(PyFileIOObject *self, PyObject *args)
{
	Py_ssize_t n;
	char *ptr;

	if (self->fd < 0)
		return err_closed();
	if (!self->writable)
		return err_mode("writing");

	if (!PyArg_ParseTuple(args, "s#", &ptr, &n))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	n = write(self->fd, ptr, n);
	Py_END_ALLOW_THREADS

	if (n < 0) {
		if (errno == EAGAIN)
			Py_RETURN_NONE;
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

	return PyInt_FromLong(n);
}

/* XXX Windows support below is likely incomplete */

#if defined(MS_WIN64) || defined(MS_WINDOWS)
typedef PY_LONG_LONG Py_off_t;
#else
typedef off_t Py_off_t;
#endif

/* Cribbed from posix_lseek() */
static PyObject *
portable_lseek(int fd, PyObject *posobj, int whence)
{
	Py_off_t pos, res;

#ifdef SEEK_SET
	/* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
	switch (whence) {
#if SEEK_SET != 0
	case 0: whence = SEEK_SET; break;
#endif
#if SEEK_CUR != 1
	case 1: whence = SEEK_CUR; break;
#endif
#if SEEL_END != 2
	case 2: whence = SEEK_END; break;
#endif
	}
#endif /* SEEK_SET */

	if (posobj == NULL)
		pos = 0;
	else {
#if !defined(HAVE_LARGEFILE_SUPPORT)
		pos = PyInt_AsLong(posobj);
#else
		pos = PyLong_Check(posobj) ?
			PyLong_AsLongLong(posobj) : PyInt_AsLong(posobj);
#endif
		if (PyErr_Occurred())
			return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
#if defined(MS_WIN64) || defined(MS_WINDOWS)
	res = _lseeki64(fd, pos, whence);
#else
	res = lseek(fd, pos, whence);
#endif
	Py_END_ALLOW_THREADS
	if (res < 0)
		return PyErr_SetFromErrno(PyExc_IOError);

#if !defined(HAVE_LARGEFILE_SUPPORT)
	return PyInt_FromLong(res);
#else
	return PyLong_FromLongLong(res);
#endif
}

static PyObject *
fileio_seek(PyFileIOObject *self, PyObject *args)
{
	PyObject *posobj;
	int whence = 0;

	if (self->fd < 0)
		return err_closed();

	if (!PyArg_ParseTuple(args, "O|i", &posobj, &whence))
		return NULL;

	return portable_lseek(self->fd, posobj, whence);
}

static PyObject *
fileio_tell(PyFileIOObject *self, PyObject *args)
{
	if (self->fd < 0)
		return err_closed();

	return portable_lseek(self->fd, NULL, 1);
}

static PyObject *
fileio_truncate(PyFileIOObject *self, PyObject *args)
{
	PyObject *posobj = NULL;
	Py_off_t pos;
	int fd, whence;

	fd = self->fd;
	if (fd < 0)
		return err_closed();
	if (!self->writable)
		return err_mode("writing");

	if (!PyArg_ParseTuple(args, "|O", &posobj))
		return NULL;

	if (posobj == Py_None)
		posobj = NULL;

	if (posobj == NULL)
		whence = 1;
	else
		whence = 0;

	posobj = portable_lseek(fd, posobj, whence);
	if (posobj == NULL)
		return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
	pos = PyInt_AsLong(posobj);
#else
	pos = PyLong_Check(posobj) ?
		PyLong_AsLongLong(posobj) : PyInt_AsLong(posobj);
#endif
	if (PyErr_Occurred()) {
		Py_DECREF(posobj);
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	pos = ftruncate(fd, pos);
	Py_END_ALLOW_THREADS

	if (pos < 0) {
		Py_DECREF(posobj);
		PyErr_SetFromErrno(PyExc_IOError);
	}

	return posobj;
}

static char *
mode_string(PyFileIOObject *self)
{
	if (self->readable) {
		if (self->writable)
			return "r+";
		else
			return "r";
	}
	else
		return "w";
}

static PyObject *
fileio_repr(PyFileIOObject *self)
{
        if (self->fd < 0)
		return PyString_FromFormat("_fileio._FileIO(-1)");

	return PyString_FromFormat("_fileio._FileIO(%d, '%s')",
				   self->fd, mode_string(self));
}

static PyObject *
fileio_isatty(PyFileIOObject *self)
{
	long res;

	if (self->fd < 0)
		return err_closed();
	Py_BEGIN_ALLOW_THREADS
	res = isatty(self->fd);
	Py_END_ALLOW_THREADS
	return PyBool_FromLong(res);
}


PyDoc_STRVAR(fileio_doc,
"file(name: str[, mode: str]) -> file IO object\n"
"\n"
"Open a file.  The mode can be 'r', 'w' or 'a' for reading (default),\n"
"writing or appending.	The file will be created if it doesn't exist\n"
"when opened for writing or appending; it will be truncated when\n"
"opened for writing.  Add a '+' to the mode to allow simultaneous\n"
"reading and writing.");

PyDoc_STRVAR(read_doc,
"read(size: int) -> bytes.  read at most size bytes, returned as bytes.\n"
"\n"
"Only makes one system call, so less data may be returned than requested\n"
"In non-blocking mode, returns None if no data is available.  On\n"
"end-of-file, returns 0.");

PyDoc_STRVAR(write_doc,
"write(b: bytes) -> int.  Write bytes b to file, return number written.\n"
"\n"
"Only makes one system call, so not all of the data may be written.\n"
"The number of bytes actually written is returned.");

PyDoc_STRVAR(fileno_doc,
"fileno() -> int. \"file descriptor\".\n"
"\n"
"This is needed for lower-level file interfaces, such the fcntl module.");

PyDoc_STRVAR(seek_doc,
"seek(offset: int[, whence: int]) -> None.  Move to new file position.\n"
"\n"
"Argument offset is a byte count.  Optional argument whence defaults to\n"
"0 (offset from start of file, offset should be >= 0); other values are 1\n"
"(move relative to current position, positive or negative), and 2 (move\n"
"relative to end of file, usually negative, although many platforms allow\n"
"seeking beyond the end of a file)."
"\n"
"Note that not all file objects are seekable.");

PyDoc_STRVAR(truncate_doc,
"truncate([size: int]) -> None.	 Truncate the file to at most size bytes.\n"
"\n"
"Size defaults to the current file position, as returned by tell().");

PyDoc_STRVAR(tell_doc,
"tell() -> int.	 Current file position");

PyDoc_STRVAR(readinto_doc,
"readinto() -> Undocumented.  Don't use this; it may go away.");

PyDoc_STRVAR(close_doc,
"close() -> None.  Close the file.\n"
"\n"
"A closed file cannot be used for further I/O operations.  close() may be\n"
"called more than once without error.  Changes the fileno to -1.");

PyDoc_STRVAR(isatty_doc,
"isatty() -> bool.  True if the file is connected to a tty device.");

PyDoc_STRVAR(seekable_doc,
"seekable() -> bool.  True if file supports random-access.");

PyDoc_STRVAR(readable_doc,
"readable() -> bool.  True if file was opened in a read mode.");

PyDoc_STRVAR(writable_doc,
"writable() -> bool.  True if file was opened in a write mode.");

static PyMethodDef fileio_methods[] = {
	{"read",     (PyCFunction)fileio_read,	   METH_VARARGS, read_doc},
	{"readinto", (PyCFunction)fileio_readinto, METH_VARARGS, readinto_doc},
	{"write",    (PyCFunction)fileio_write,	   METH_VARARGS, write_doc},
	{"seek",     (PyCFunction)fileio_seek,	   METH_VARARGS, seek_doc},
	{"tell",     (PyCFunction)fileio_tell,	   METH_VARARGS, tell_doc},
	{"truncate", (PyCFunction)fileio_truncate, METH_VARARGS, truncate_doc},
	{"close",    (PyCFunction)fileio_close,	   METH_NOARGS,	 close_doc},
	{"seekable", (PyCFunction)fileio_seekable, METH_NOARGS,	 seekable_doc},
	{"readable", (PyCFunction)fileio_readable, METH_NOARGS,	 readable_doc},
	{"writable", (PyCFunction)fileio_writable, METH_NOARGS,	 writable_doc},
	{"fileno",   (PyCFunction)fileio_fileno,   METH_NOARGS,	 fileno_doc},
	{"isatty",   (PyCFunction)fileio_isatty,   METH_NOARGS,	 isatty_doc},
	{NULL,	     NULL}	       /* sentinel */
};

/* 'closed' and 'mode' are attributes for backwards compatibility reasons. */

static PyObject *
get_closed(PyFileIOObject *self, void *closure)
{
	return PyBool_FromLong((long)(self->fd < 0));
}

static PyObject *
get_mode(PyFileIOObject *self, void *closure)
{
	return PyString_FromString(mode_string(self));
}

static PyGetSetDef fileio_getsetlist[] = {
	{"closed", (getter)get_closed, NULL, "True if the file is closed"},
	{"mode", (getter)get_mode, NULL, "String giving the file mode"},
	{0},
};

PyTypeObject PyFileIO_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"FileIO",
	sizeof(PyFileIOObject),
	0,
	(destructor)fileio_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)fileio_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	fileio_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PyFileIOObject, weakreflist),	/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	fileio_methods,				/* tp_methods */
	0,					/* tp_members */
	fileio_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	fileio_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	fileio_new,				/* tp_new */
	PyObject_Del,				/* tp_free */
};

static PyMethodDef module_methods[] = {
	{NULL, NULL}
};

PyMODINIT_FUNC
init_fileio(void)
{
	PyObject *m;	/* a module object */

	m = Py_InitModule3("_fileio", module_methods,
			   "Fast implementation of io.FileIO.");
	if (m == NULL)
		return;
	if (PyType_Ready(&PyFileIO_Type) < 0)
		return;
	Py_INCREF(&PyFileIO_Type);
	PyModule_AddObject(m, "_FileIO", (PyObject *) &PyFileIO_Type);
}
