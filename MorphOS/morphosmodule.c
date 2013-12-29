/*******************************************************************************
 *** $Id$
 ***
 *** Morphos module implementation (adapted from posixmodule.c)
 ***
 *******************************************************************************
 */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structseq.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <grp.h>

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif /* HAVE_SYSEXITS_H */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#include <sys/times.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#include "osdefs.h"

#include "morphos.h"


/*
** System Includes
*/

#include <exec/system.h>

#include <clib/netlib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/usergroup.h>
#include <proto/alib.h>
#include <proto/timer.h> // used by morphos_urandom

#include <exec/lists.h>


/*
** Private Macros and Definitions
*/

#ifndef MAXPATHLEN
#if defined(PATH_MAX) && PATH_MAX > 1024
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 1024
#endif
#endif /* MAXPATHLEN */

#define WAIT_TYPE int
#define WAIT_STATUS_INT(s) (s)

/* Issue #1983: pid_t can be longer than a C long on some systems */
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
#define PARSE_PID "i"
#define PyLong_FromPid PyLong_FromLong
#define PyLong_AsPid PyLong_AsLong
#elif SIZEOF_PID_T == SIZEOF_LONG
#define PARSE_PID "l"
#define PyLong_FromPid PyLong_FromLong
#define PyLong_AsPid PyLong_AsLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
#define PARSE_PID "L"
#define PyLong_FromPid PyLong_FromLongLong
#define PyLong_AsPid PyLong_AsLongLong
#else
#error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */

/* choose the appropriate stat and fstat functions and return structs */
#undef STAT
#define STAT stat
#define FSTAT fstat
#define STRUCT_STAT struct stat

#undef GETPGRP_HAVE_ARG

#define _PyVerify_fd_dup2(A, B) (1)

PyDoc_STRVAR(morphos__doc__,
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the Morphos standard (ABox). \n\
Refer to the library manual and corresponding Morphos manual entries \n\
for more information on calls.");

extern void __seterrno(void); // this function set errno depend on IoErr() return value

/* Return a dictionary corresponding to the AmigaDOS environment dictionary.
**
** That is, scan ENV: for global environment variables.
** The local shell environment variables and aliases are put into others dictionaries.
*/
static BOOL
convertenviron(
    PyObject **dglobal,
    PyObject **dlocal,
    PyObject **dboth,
    PyObject **daliases
    )
{
    BPTR lock;
    struct List *localVars;

    *dglobal  = PyDict_New();
    *dlocal   = PyDict_New();
    *dboth    = PyDict_New();
    *daliases = PyDict_New();

    if (!(*dglobal && *dlocal && *dboth && *daliases))
    {
        Py_XDECREF(*dglobal);
        Py_XDECREF(*dlocal);
        Py_XDECREF(*dboth);
        Py_XDECREF(*daliases);
        return FALSE;
    }

    /* Read global vars from ENV:
    ** Put them in 'dglob' and in 'dboth'.
    */

    lock = Lock("ENV:", ACCESS_READ);
    if (NULL != lock) {
        struct FileInfoBlock fib;

        /* Obtain a FileInfoBlock on 'ENV:' */
        if (Examine(lock, &fib)) {

            /* Parse directory entries */
            while (ExNext(lock, &fib)) {

                /* Is it a file ? */
                if (fib.fib_DirEntryType < 0) {
                    char * buffer;

                    buffer = PyMem_Malloc(fib.fib_Size + 1);
                    if (NULL != buffer) {
                        int len;

                        len = GetVar(fib.fib_FileName, buffer, fib.fib_Size + 1, GVF_GLOBAL_ONLY);
                        if (len >= 0) {
                            PyObject *k;
                            PyObject *v;

                            /* Latin-1 char buffer converted as bytes object.
                             * Note: exceptions are ignored, that may result in partially filled dicts.
                             */

                            k = PyBytes_FromStringAndSize(fib.fib_FileName, strlen(fib.fib_FileName));
                            if (NULL == k) {
                                PyErr_Clear();
                                continue;
                            }

                            v = PyBytes_FromStringAndSize(buffer, len);
                            if (NULL == v) {
                                PyErr_Clear();
                                Py_DECREF(k);
                                continue;
                            }

                            /* Prevent overwritting */
                            if (PyDict_GetItem(*dglobal, k) == NULL) {
                                /* Fill 'global' and 'both' dicts (ignoring exceptions) */

                                if (PyDict_SetItem(*dglobal, k, v) != 0)
                                    PyErr_Clear();

                                if (PyDict_SetItem(*dboth, k, v) != 0)
                                    PyErr_Clear();
                            }

                            Py_DECREF(k);
                            Py_DECREF(v);
                        }

                        PyMem_Free(buffer);
                    }
                }
            }
        }

        UnLock(lock);
    }

    /* Now, loop on local shell environment variables and aliases.
    ** Put variables in 'dlocal' and 'dboth'.
    ** Put aliases in 'daliases'.
    **
    ** Note: local shell variables are put in 'dboth' after globals. So, if
    ** there is a local variable with the same name that a global, the
    ** global is overwriten by the local, as it should be.
    */

    /* Normally not needed to lock this list access, because only the process
     * itself shall modify it.
     */
    localVars = (struct List *) &((struct Process *)FindTask(NULL))->pr_LocalVars;
    if(!IsListEmpty(localVars)) {
        struct LocalVar *var;

        ForeachNode(localVars, var) {
            PyObject *k;
            PyObject *v;

            k = PyBytes_FromStringAndSize(var->lv_Node.ln_Name, strlen(var->lv_Node.ln_Name));
            if (NULL == k) {
                PyErr_Clear();
                continue;
            }
            
            v = PyBytes_FromStringAndSize(var->lv_Value, var->lv_Len);
            if (NULL == v) {
                PyErr_Clear();
                Py_DECREF(k);
                continue;
            }
            
            /* Put it in the right dict (like globals) */
            switch (var->lv_Node.ln_Type) {
                case LV_VAR:
                    if (NULL == PyDict_GetItem(*dlocal, k)) {
                        if (PyDict_SetItem(*dlocal, k, v) != 0)
                            PyErr_Clear();

                        if (PyDict_SetItem(*dboth, k, v) != 0)
                            PyErr_Clear();
                    }
                    break;

                case LV_ALIAS:
                    if (NULL == PyDict_GetItem(*daliases, k))
                        if (PyDict_SetItem(*daliases, k, v) != 0)
                            PyErr_Clear();
                    break;

                default:;
            }
            
            Py_DECREF(k);
            Py_DECREF(v);
        }
    }

    return TRUE;
}
/***********************************************************
** Set a MorphOS-specific error from errno, and return NULL
*/
static PyObject *
morphos_error(void)
{
    if (IoErr())
        errno = -1;
    return PyErr_SetFromErrno(PyExc_OSError);
}
static PyObject *
morphos_error_with_filename(char* name)
{
    if (IoErr())
        errno = -1;
    return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
}
static PyObject *
morphos_error_with_allocated_filename(PyObject* name)
{
    PyObject *name_str, *rc;
    
    if (IoErr())
        errno = -1;
    name_str = PyUnicode_DecodeFSDefaultAndSize(PyBytes_AsString(name),
                                                PyBytes_GET_SIZE(name));
    Py_DECREF(name);
    rc = PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError,
                                              name_str);
    Py_XDECREF(name_str);
    return rc;
}
/**************************
** MorphOS generic methods
*/
static PyObject *
morphos_fildes(PyObject *fdobj, int (*func)(int))
{
    int fd;
    int res;
    fd = PyObject_AsFileDescriptor(fdobj);
    if (fd < 0)
        return NULL;
    if (!_PyVerify_fd(fd))
		return morphos_error();
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}
static PyObject *
morphos_1str(PyObject *args, char *format, int (*func)(const char*))
{
    PyObject *opath1 = NULL;
    char *path1;
    int res;
    if (!PyArg_ParseTuple(args, format,
                          PyUnicode_FSConverter, &opath1))
        return NULL;
    path1 = PyBytes_AsString(opath1);
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(opath1);
    Py_DECREF(opath1);
    Py_INCREF(Py_None);
    return Py_None;
}
static PyObject *
morphos_2str(PyObject *args,
             char *format,
             int (*func)(const char *, const char *))
{
    PyObject *opath1 = NULL, *opath2 = NULL;
    char *path1, *path2;
    int res;

    if (!PyArg_ParseTuple(args, format,
                          PyUnicode_FSConverter, &opath1,
                          PyUnicode_FSConverter, &opath2))
        return NULL;
    path1 = PyBytes_AsString(opath1);
	path2 = PyBytes_AsString(opath2);
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1, path2);
    Py_END_ALLOW_THREADS
    Py_DECREF(opath1);
	Py_DECREF(opath2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}
PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
If your platform supports st_blksize, st_blocks, st_rdev,\n\
or st_flags, they are available as attributes only.\n\
\n\
See os.stat for more information.");

static PyStructSequence_Field stat_result_fields[] = {
	{"st_mode",    "protection bits"},
	{"st_ino",     "inode"},
	{"st_dev",     "device"},
	{"st_nlink",   "number of hard links"},
	{"st_uid",     "user ID of owner"},
	{"st_gid",     "group ID of owner"},
	{"st_size",    "total size, in bytes"},
	/* The NULL is replaced with PyStructSequence_UnnamedField later. */
	{NULL,   "integer time of last access"},
	{NULL,   "integer time of last modification"},
	{NULL,   "integer time of last change"},
	{"st_atime",   "time of last access"},
	{"st_mtime",   "time of last modification"},
	{"st_ctime",   "time of last change"},
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	{"st_blksize", "blocksize for filesystem I/O"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	{"st_blocks",  "number of blocks allocated"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	{"st_rdev",    "device type (if inode device)"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
	{"st_flags",   "user defined flags for file"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
	{"st_gen",    "generation number"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
	{"st_birthtime",   "time of creation"},
#endif
	{0}
};

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
#define ST_BLKSIZE_IDX 13
#else
#define ST_BLKSIZE_IDX 12
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
#define ST_BLOCKS_IDX (ST_BLKSIZE_IDX+1)
#else
#define ST_BLOCKS_IDX ST_BLKSIZE_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
#define ST_RDEV_IDX (ST_BLOCKS_IDX+1)
#else
#define ST_RDEV_IDX ST_BLOCKS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_FLAGS
#define ST_FLAGS_IDX (ST_RDEV_IDX+1)
#else
#define ST_FLAGS_IDX ST_RDEV_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_GEN
#define ST_GEN_IDX (ST_FLAGS_IDX+1)
#else
#define ST_GEN_IDX ST_FLAGS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
#define ST_BIRTHTIME_IDX (ST_GEN_IDX+1)
#else
#define ST_BIRTHTIME_IDX ST_GEN_IDX
#endif
static PyStructSequence_Desc stat_result_desc = {
    "stat_result", /* name */
    stat_result__doc__, /* doc */
    stat_result_fields,
    10
};

PyDoc_STRVAR(statvfs_result__doc__,
"statvfs_result: Result from statvfs or fstatvfs.\n\n\
This object may be accessed either as a tuple of\n\
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),\n\
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.\n\
\n\
See os.statvfs for more information.");

static PyStructSequence_Field statvfs_result_fields[] = {
        {"f_bsize",  },
        {"f_frsize", },
        {"f_blocks", },
        {"f_bfree",  },
        {"f_bavail", },
        {"f_files",  },
        {"f_ffree",  },
        {"f_favail", },
        {"f_flag",   },
        {"f_namemax",},
        {0}
};
static PyStructSequence_Desc statvfs_result_desc = {
    "statvfs_result", /* name */
    statvfs_result__doc__, /* doc */
    statvfs_result_fields,
    10
};

static int initialized;
static PyTypeObject StatResultType;
static PyTypeObject StatVFSResultType;
static newfunc structseq_new;

static PyObject *
statresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyStructSequence *result;
	int i;

	result = (PyStructSequence*)structseq_new(type, args, kwds);
	if (!result)
		return NULL;
	/* If we have been initialized from a tuple,
	   st_?time might be set to None. Initialize it
	   from the int slots.  */
	for (i = 7; i <= 9; i++) {
		if (result->ob_item[i+3] == Py_None) {
			Py_DECREF(Py_None);
			Py_INCREF(result->ob_item[i]);
			result->ob_item[i+3] = result->ob_item[i];
		}
	}
	return (PyObject*)result;
}

/* If true, st_?time is float. */
static int _stat_float_times = 0;

PyDoc_STRVAR(stat_float_times__doc__,
"stat_float_times([newval]) -> oldval\n\n\
Determine whether os.[lf]stat represents time stamps as float objects.\n\
If newval is True, future calls to stat() return floats, if it is False,\n\
future calls return ints. \n\
If newval is omitted, return the current setting.\n");

static PyObject*
stat_float_times(PyObject* self, PyObject *args)
{
	int newval = -1;
	if (!PyArg_ParseTuple(args, "|i:stat_float_times", &newval))
		return NULL;
	if (newval == -1)
		/* Return old value */
		return PyBool_FromLong(_stat_float_times);
	_stat_float_times = newval;
	Py_INCREF(Py_None);
	return Py_None;
}

static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
	PyObject *fval,*ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
	ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
	ival = PyLong_FromLong((long)sec);
#endif
	if (!ival)
		return;
	if (_stat_float_times) {
		fval = PyFloat_FromDouble(sec + 1e-9*nsec);
	} else {
		fval = ival;
		Py_INCREF(fval);
	}
	PyStructSequence_SET_ITEM(v, index, ival);
	PyStructSequence_SET_ITEM(v, index+3, fval);
}
/* pack a system stat C structure into the Python stat tuple
   (used by morphos_stat() and morphos_fstat()) */
static PyObject*
_pystat_fromstructstat(STRUCT_STAT *st)
{
	unsigned long ansec, mnsec, cnsec;
	PyObject *v = PyStructSequence_New(&StatResultType);
	if (v == NULL)
		return NULL;

        PyStructSequence_SET_ITEM(v, 0, PyLong_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyStructSequence_SET_ITEM(v, 1,
				  PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
        PyStructSequence_SET_ITEM(v, 1, PyLong_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG) && !defined(MS_WINDOWS)
        PyStructSequence_SET_ITEM(v, 2,
				  PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
        PyStructSequence_SET_ITEM(v, 2, PyLong_FromLong((long)st->st_dev));
#endif
        PyStructSequence_SET_ITEM(v, 3, PyLong_FromLong((long)st->st_nlink));
        PyStructSequence_SET_ITEM(v, 4, PyLong_FromLong((long)st->st_uid));
        PyStructSequence_SET_ITEM(v, 5, PyLong_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyStructSequence_SET_ITEM(v, 6,
				  PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
        PyStructSequence_SET_ITEM(v, 6, PyLong_FromLong(st->st_size));
#endif

#if defined(HAVE_STAT_TV_NSEC)
	ansec = st->st_atim.tv_nsec;
	mnsec = st->st_mtim.tv_nsec;
	cnsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STAT_TV_NSEC2)
	ansec = st->st_atimespec.tv_nsec;
	mnsec = st->st_mtimespec.tv_nsec;
	cnsec = st->st_ctimespec.tv_nsec;
#elif defined(HAVE_STAT_NSEC)
	ansec = st->st_atime_nsec;
	mnsec = st->st_mtime_nsec;
	cnsec = st->st_ctime_nsec;
#else
	ansec = mnsec = cnsec = 0;
#endif
	fill_time(v, 7, st->st_atime, ansec);
	fill_time(v, 8, st->st_mtime, mnsec);
	fill_time(v, 9, st->st_ctime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
			 PyLong_FromLong((long)st->st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
			 PyLong_FromLong((long)st->st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
			 PyLong_FromLong((long)st->st_rdev));
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
	PyStructSequence_SET_ITEM(v, ST_GEN_IDX,
			 PyLong_FromLong((long)st->st_gen));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
	{
	  PyObject *val;
	  unsigned long bsec,bnsec;
	  bsec = (long)st->st_birthtime;
#ifdef HAVE_STAT_TV_NSEC2
	  bnsec = st->st_birthtimespec.tv_nsec;
#else
	  bnsec = 0;
#endif
	  if (_stat_float_times) {
	    val = PyFloat_FromDouble(bsec + 1e-9*bnsec);
	  } else {
	    val = PyLong_FromLong((long)bsec);
	  }
	  PyStructSequence_SET_ITEM(v, ST_BIRTHTIME_IDX,
				    val);
	}
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
	PyStructSequence_SET_ITEM(v, ST_FLAGS_IDX,
			 PyLong_FromLong((long)st->st_flags));
#endif

	if (PyErr_Occurred()) {
		Py_DECREF(v);
		return NULL;
	}

	return v;
}
static PyObject *
morphos_do_stat(PyObject *self, PyObject *args,
                char *format,
                int (*statfunc)(const char *, STRUCT_STAT *),
                char *wformat,
                int (*wstatfunc)(const Py_UNICODE *, STRUCT_STAT *))
{
    STRUCT_STAT st;
    PyObject *opath;
    char *path = NULL;  /* pass this to stat; do not free() it */
    int res;
    PyObject *result;

    if (!PyArg_ParseTuple(args, format, PyUnicode_FSConverter, &opath))
	    return NULL;

    path = PyBytes_AsString(opath);
    Py_BEGIN_ALLOW_THREADS
    res = (*statfunc)(path, &st);
    Py_END_ALLOW_THREADS

    if (res != 0)
	    result = morphos_error_with_filename(path);
    else
	    result = _pystat_fromstructstat(&st);

    Py_DECREF(opath);
    return result;
}
/*************************
** MorphOS module methods
*/

PyDoc_STRVAR(morphos_access__doc__,
"access(path, mode) -> True if granted, False otherwise\n\n\
Use the real uid/gid to test for access to a path.  Note that most\n\
operations will use the effective uid/gid, therefore this routine can\n\
be used in a suid/sgid environment to test if the invoking user has the\n\
specified access to the path.  The mode argument can be F_OK to test\n\
existence, or the inclusive-OR of R_OK, W_OK, and X_OK.");

static PyObject *
morphos_access(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    int mode;
    int res;

	if (!PyArg_ParseTuple(args, "O&i:access", 
                          PyUnicode_FSConverter, &opath, &mode))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = access(path, mode);
	Py_END_ALLOW_THREADS
	Py_DECREF(opath);
	return PyBool_FromLong(res == 0);
}
#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

#ifdef HAVE_CTERMID
//+ morphos_ctermid
PyDoc_STRVAR(morphos_ctermid__doc__,
"ctermid() -> string\n\n\
Return the name of the controlling terminal for this process.");

static PyObject *
morphos_ctermid(PyObject *self, PyObject *noargs)
{
    char *ret;
    char buffer[L_ctermid];

#ifdef USE_CTERMID_R
    ret = ctermid_r(buffer);
#else
    ret = ctermid(buffer);
#endif
	if (ret == NULL)
		return morphos_error();
	return PyUnicode_DecodeLatin1(buffer, strlen(buffer), "surrogateescape");
}
//-
#endif

PyDoc_STRVAR(morphos_chdir__doc__,
"chdir(path)\n\n\
Change the current working directory to the specified path.");

static PyObject *
morphos_chdir(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "O&:chdir", chdir);
}
PyDoc_STRVAR(morphos_chmod__doc__,
"chmod(path, mode)\n\n\
Change the access permissions of a file.");

static PyObject *
morphos_chmod(PyObject *self, PyObject *args)
{
	PyObject *opath = NULL;
	char *path = NULL;
	int i;
	int res;

    if (!PyArg_ParseTuple(args, "O&i:chmod", PyUnicode_FSConverter,
	                      &opath, &i))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = chmod(path, i);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}
#ifdef HAVE_LCHMOD
//+ morphos_lchmod()
PyDoc_STRVAR(morphos_lchmod__doc__,
"lchmod(path, mode)\n\n\
Change the access permissions of a file. If path is a symlink, this\n\
affects the link itself rather than the target.");

static PyObject *
morphos_lchmod(PyObject *self, PyObject *args)
{
	PyObject *opath;
	char *path;
	int i;
	int res;
	if (!PyArg_ParseTuple(args, "O&i:lchmod", PyUnicode_FSConverter,
	                      &opath, &i))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = lchmod(path, i);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_RETURN_NONE;
}
//-
#endif /* HAVE_LCHMOD */

#ifdef HAVE_CHFLAGS
//+ morphos_chflags()
PyDoc_STRVAR(morphos_chflags__doc__,
"chflags(path, flags)\n\n\
Set file flags.");

static PyObject *
morphos_chflags(PyObject *self, PyObject *args)
{
	PyObject *opath;
	char *path;
	unsigned long flags;
	int res;
	if (!PyArg_ParseTuple(args, "O&k:chflags",
			      PyUnicode_FSConverter, &opath, &flags))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = chflags(path, flags);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}
//-
#endif /* HAVE_CHFLAGS */

#ifdef HAVE_LCHFLAGS
//+ morphos_lchflags
PyDoc_STRVAR(morphos_lchflags__doc__,
"lchflags(path, flags)\n\n\
Set file flags.\n\
This function will not follow symbolic links.");

static PyObject *
morphos_lchflags(PyObject *self, PyObject *args)
{
	PyObject *opath;
	char *path;
	unsigned long flags;
	int res;
	if (!PyArg_ParseTuple(args, "O&k:lchflags",
			      PyUnicode_FSConverter, &opath, &flags))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = lchflags(path, flags);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}
//-
#endif /* HAVE_LCHFLAGS */

#ifdef HAVE_CHROOT
//+ morphos_chroot()
PyDoc_STRVAR(morphos_chroot__doc__,
"chroot(path)\n\n\
Change root directory to path.");

static PyObject *
morphos_chroot(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "O&:chroot", chroot);
}
//-
#endif

#ifdef _HAVE_FSYNC
//+ morphos_fsync()
PyDoc_STRVAR(morphos_fsync__doc__,
"fsync(fildes)\n\n\
force write of file with filedescriptor to disk.");

static PyObject *
morphos_fsync(PyObject *self, PyObject *fdobj)
{
    return morphos_fildes(fdobj, fsync);
}
//-
#endif /* HAVE_FSYNC */

#ifdef _HAVE_FDATASYNC
//+ morphos_fdatasync()
PyDoc_STRVAR(morphos_fdatasync__doc__,
"fdatasync(fildes)\n\n\
force write of file with filedescriptor to disk.\n\
 does not force update of metadata.");

static PyObject *
morphos_fdatasync(PyObject *self, PyObject *fdobj)
{
       return morphos_fildes(fdobj, fdatasync);
}
//-
#endif /* HAVE_FDATASYNC */

PyDoc_STRVAR(morphos_chown__doc__,
"chown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.");

static PyObject *
morphos_chown(PyObject *self, PyObject *args)
{
	PyObject *opath;
	char *path;
	long uid, gid;
	int res;
	if (!PyArg_ParseTuple(args, "O&ll:chown",
	                      PyUnicode_FSConverter, &opath,
	                      &uid, &gid))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = chown(path, (uid_t) uid, (gid_t) gid);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}
#ifdef HAVE_LCHOWN
//+ morphos_lchown()
PyDoc_STRVAR(morphos_lchown__doc__,
"lchown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
This function will not follow symbolic links.");

static PyObject *
morphos_lchown(PyObject *self, PyObject *args)
{
	PyObject *opath;
	char *path;
	int uid, gid;
	int res;
	if (!PyArg_ParseTuple(args, "O&ii:lchown",
	                      PyUnicode_FSConverter, &opath,
	                      &uid, &gid))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = lchown(path, (uid_t) uid, (gid_t) gid);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}//-
#endif /* HAVE_LCHOWN */

static PyObject *
morphos_getcwd(int use_bytes)
{
	char buf[1026];
	char *res;

    Py_BEGIN_ALLOW_THREADS
	res = getcwd(buf, sizeof buf);
	Py_END_ALLOW_THREADS
	if (res == NULL)
		return morphos_error();
	if (use_bytes)
		return PyBytes_FromStringAndSize(buf, strlen(buf));
	return PyUnicode_Decode(buf, strlen(buf), Py_FileSystemDefaultEncoding,"surrogateescape");

}
PyDoc_STRVAR(morphos_getcwd__doc__,
"getcwd() -> path\n\n\
Return a unicode string representing the current working directory.");

static PyObject *
morphos_getcwd_unicode(PyObject *self)
{
    return morphos_getcwd(0);
}
PyDoc_STRVAR(morphos_getcwdb__doc__,
"getcwdb() -> path\n\n\
Return a bytes string representing the current working directory.");

static PyObject *
morphos_getcwd_bytes(PyObject *self)
{
    return morphos_getcwd(1);
}
PyDoc_STRVAR(morphos_link__doc__,
"link(src, dst)\n\n\
Create a hard link to a file.");

static PyObject *
morphos_link(PyObject *self, PyObject *args)
{
    PyObject *opath1 = NULL, *opath2 = NULL;
    char *path1 = NULL, *path2 = NULL;
    int res = -1;
    BPTR lock;

	if (!PyArg_ParseTuple(args, "O&O&:link",
                          PyUnicode_FSConverter, &opath1,
                          PyUnicode_FSConverter, &opath2))
        return NULL;

    path1 = PyBytes_AsString(opath1);
	path2 = PyBytes_AsString(opath2);

    Py_BEGIN_ALLOW_THREADS
    lock = Lock(path1, SHARED_LOCK);
    if (NULL != lock) {
        res = !MakeLink(path2, lock, FALSE);
        if (res)
            __seterrno();
        UnLock(lock);
    } else
        __seterrno();
    Py_END_ALLOW_THREADS

    Py_DECREF(opath1);
	Py_DECREF(opath2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}
PyDoc_STRVAR(morphos_listdir__doc__,
"listdir(path) -> list_of_strings\n\n\
Return a list containing the names of the entries in the directory.\n\
\n\
    path: path of directory to list\n\
\n\
The list is in arbitrary order.");

static PyObject *
morphos_listdir(PyObject *self, PyObject *args)
{
    /* XXX Should redo this putting the (now four) versions of opendir
       in separate files instead of having them all here... */
    PyObject *oname=NULL;
    PyObject *d=NULL, *v=NULL;
    int arg_is_unicode = 1;

    errno = 0;
	if (!PyArg_ParseTuple(args, "|U:listdir", &v)) {
		arg_is_unicode = 0;
		PyErr_Clear();
	}
    if (PyArg_ParseTuple(args, "|O&:listdir", PyUnicode_FSConverter, &oname)) {
        BPTR lock;
        char *name;
        
        if (oname)
            name = PyBytes_AsString(oname);
        else
            name = "";

        Py_BEGIN_ALLOW_THREADS
        lock = Lock(name, SHARED_LOCK);
        Py_END_ALLOW_THREADS
        
        if (NULL != lock) {
            struct FileInfoBlock fib;
            LONG res;
        
            Py_BEGIN_ALLOW_THREADS
            res = Examine64Tags(lock, &fib, TAG_DONE);
            Py_END_ALLOW_THREADS

            if (res) {
                d = PyList_New(0);
                if (NULL != d) {
                    while (1) {
                        Py_BEGIN_ALLOW_THREADS
                        res = ExNext64Tags(lock, &fib, NULL);
                        Py_END_ALLOW_THREADS

                        if (res == 0) {
                            if (IoErr() != ERROR_NO_MORE_ENTRIES) {
                                Py_CLEAR(d);
                                morphos_error_with_allocated_filename(oname);
                                oname = NULL;
                            }
                            
                            break;
                        }

                        v = PyBytes_FromStringAndSize(fib.fib_FileName, strlen(fib.fib_FileName));
                        if (NULL == v) {
                            Py_CLEAR(d);
                            break;
                        }

                        if (arg_is_unicode) {
                            PyObject *w;

                            w = PyUnicode_FromEncodedObject(v,
                                                            Py_FileSystemDefaultEncoding,
                                                            "surrogateescape");
                            Py_DECREF(v);
                            if (w != NULL)
                                v = w;
                            else {
                                /* Encoding failed to decode ASCII bytes.
                                   Raise exception. */
                                Py_CLEAR(d);
                                break;
                            }
                        }

                        if (PyList_Append(d, v) != 0) {
                            Py_DECREF(v);
                            Py_CLEAR(d);
                            break;
                        }

                        Py_DECREF(v);
                    }
                }
            } else {
                morphos_error_with_allocated_filename(oname);
                oname = NULL;
            }

            Py_BEGIN_ALLOW_THREADS
            UnLock(lock);
            Py_END_ALLOW_THREADS
            
        } else {
            morphos_error_with_allocated_filename(oname);
            oname = NULL;
        }

        Py_XDECREF(oname);
    }

    return d;
}
PyDoc_STRVAR(morphos_mkdir__doc__,
"mkdir(path [, mode=0777])\n\n\
Create a directory.");

static PyObject *
morphos_mkdir(PyObject *self, PyObject *args)
{
	int res;
	PyObject *opath;
	char *path;
	int mode = 0777;

    if (!PyArg_ParseTuple(args, "O&|i:mkdir",
	                      PyUnicode_FSConverter, &opath, &mode))
		return NULL;
	path = PyBytes_AsString(opath);
	Py_BEGIN_ALLOW_THREADS
	res = mkdir(path, mode);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error_with_allocated_filename(opath);
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(morphos_nice__doc__,
"nice(inc) -> new_priority\n\n\
Decrease the priority of process by inc and return the new priority.");

static PyObject *
morphos_nice(PyObject *self, PyObject *args)
{
    LONG current;
    int increment;

    if (!PyArg_ParseTuple(args, "i:nice", &increment))
        return NULL;

    if (!NewSetTaskAttrsA(NULL, &increment, sizeof(increment), TASKINFOTYPE_NICE, NULL))
        return PyErr_Format(PyExc_OSError, "unable to set TASKINFOTYPE_NICE attribute on current process");
        
    if (!NewGetTaskAttrsA(NULL, &current, sizeof(current), TASKINFOTYPE_NICE, NULL))
        return PyErr_Format(PyExc_OSError, "unable to get TASKINFOTYPE_NICE attribute on current process");

    return PyLong_FromLong(current);
}

PyDoc_STRVAR(morphos_rename__doc__,
"rename(old, new)\n\n\
Rename a file or directory.");

static PyObject *
morphos_rename(PyObject *self, PyObject *args)
{
    return morphos_2str(args, "O&O&:rename", rename);
}
PyDoc_STRVAR(morphos_rmdir__doc__,
"rmdir(path)\n\n\
Remove a directory.");

static PyObject *
morphos_rmdir(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "O&:rmdir", rmdir);
}
PyDoc_STRVAR(morphos_stat__doc__,
"stat(path) -> stat result\n\n\
Perform a stat system call on the given path.");

static PyObject *
morphos_stat(PyObject *self, PyObject *args)
{
    return morphos_do_stat(self, args, "O&:stat", STAT, NULL, NULL);
}
PyDoc_STRVAR(morphos_system__doc__,
"system(command) -> exit_status\n\n\
Execute the command (a string) in a subshell.");

static PyObject *
morphos_system(PyObject *self, PyObject *args)
{
    long sts;
    char *command;
    
    if (!PyArg_ParseTuple(args, "s:system", &command))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    sts = SystemTagList(command, NULL);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(sts);
}
PyDoc_STRVAR(morphos_umask__doc__,
"umask(new_mask) -> old_mask\n\n\
Set the current numeric umask and return the previous umask.");

static PyObject *
morphos_umask(PyObject *self, PyObject *args)
{
    int i;
    if (!PyArg_ParseTuple(args, "i:umask", &i))
        return NULL;
    i = (int)umask(i);
    if (i < 0)
        return morphos_error();
    return PyLong_FromLong((long)i);
}
PyDoc_STRVAR(morphos_unlink__doc__,
"unlink(path)\n\n\
Remove a file (same as remove(path)).");

PyDoc_STRVAR(morphos_remove__doc__,
"remove(path)\n\n\
Remove a file (same as unlink(path)).");

static PyObject *
morphos_unlink(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "O&:remove", unlink);
}
#ifdef HAVE_UNAME
//+ morphos_uname()
PyDoc_STRVAR(morphos_uname__doc__,
"uname() -> (sysname, nodename, release, version, machine)\n\n\
Return a tuple identifying the current operating system.");

static PyObject *
morphos_uname(PyObject *self, PyObject *noargs)
{
    struct utsname u;
    int res;

    Py_BEGIN_ALLOW_THREADS
    res = uname(&u);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    return Py_BuildValue("(sssss)",
                 u.sysname,
                 u.nodename,
                 u.release,
                 u.version,
                 u.machine);
}
//-
#endif /* HAVE_UNAME */

static int
extract_time(PyObject *t, long* sec, long* usec)
{
	long intval;
	if (PyFloat_Check(t)) {
		double tval = PyFloat_AsDouble(t);
		PyObject *intobj = Py_TYPE(t)->tp_as_number->nb_int(t);
		if (!intobj)
			return -1;
		intval = PyLong_AsLong(intobj);
		Py_DECREF(intobj);
		if (intval == -1 && PyErr_Occurred())
			return -1;
		*sec = intval;
		*usec = (long)((tval - intval) * 1e6); /* can't exceed 1000000 */
		if (*usec < 0)
			/* If rounding gave us a negative number,
			   truncate.  */
			*usec = 0;
		return 0;
	}
	intval = PyLong_AsLong(t);
	if (intval == -1 && PyErr_Occurred())
		return -1;
	*sec = intval;
	*usec = 0;
        return 0;
}
PyDoc_STRVAR(morphos_utime__doc__,
"utime(path, (atime, utime))\n\
utime(path, None)\n\n\
Set the access and modified time of the file to the given values.  If the\n\
second form is used, set the access and modified times to the current time.");

static PyObject *
morphos_utime(PyObject *self, PyObject *args)
{
    PyObject *opath;
	char *path;
	long atime, mtime, ausec, musec;
	int res;
	PyObject* arg;

#if defined(HAVE_UTIMES)
	struct timeval buf[2];
#define ATIME buf[0].tv_sec
#define MTIME buf[1].tv_sec
#elif defined(HAVE_UTIME_H)
/* XXX should define struct utimbuf instead, above */
	struct utimbuf buf;
#define ATIME buf.actime
#define MTIME buf.modtime
#define UTIME_ARG &buf
#else /* HAVE_UTIMES */
	time_t buf[2];
#define ATIME buf[0]
#define MTIME buf[1]
#define UTIME_ARG buf
#endif /* HAVE_UTIMES */

	if (!PyArg_ParseTuple(args, "O&O:utime",
                          PyUnicode_FSConverter, &opath, &arg))
		return NULL;
	path = PyBytes_AsString(opath);
	if (arg == Py_None) {
		/* optional time values not given */
		Py_BEGIN_ALLOW_THREADS
		res = utime(path, NULL);
		Py_END_ALLOW_THREADS
	}
	else if (!PyTuple_Check(arg) || PyTuple_Size(arg) != 2) {
		PyErr_SetString(PyExc_TypeError,
				"utime() arg 2 must be a tuple (atime, mtime)");
		Py_DECREF(opath);
		return NULL;
	} else {
		if (extract_time(PyTuple_GET_ITEM(arg, 0),
				 &atime, &ausec) == -1) {
			Py_DECREF(opath);
			return NULL;
		}
		if (extract_time(PyTuple_GET_ITEM(arg, 1),
				 &mtime, &musec) == -1) {
			Py_DECREF(opath);
			return NULL;
		}
		ATIME = atime;
		MTIME = mtime;
#ifdef HAVE_UTIMES
		buf[0].tv_usec = ausec;
		buf[1].tv_usec = musec;
		Py_BEGIN_ALLOW_THREADS
		res = utimes(path, buf);
		Py_END_ALLOW_THREADS
#else
		Py_BEGIN_ALLOW_THREADS
		res = utime(path, UTIME_ARG);
		Py_END_ALLOW_THREADS
#endif /* HAVE_UTIMES */
	}
	if (res < 0) {
		return morphos_error_with_allocated_filename(opath);
	}
	Py_DECREF(opath);
	Py_INCREF(Py_None);
	return Py_None;
#undef UTIME_ARG
#undef ATIME
#undef MTIME
}
/*********************
** Process operations
*/

PyDoc_STRVAR(morphos__exit__doc__,
"_exit(status)\n\n\
Exit to the system with specified status, without normal exit processing.");

static PyObject *
morphos__exit(PyObject *self, PyObject *args)
{
    int sts;
    if (!PyArg_ParseTuple(args, "i:_exit", &sts))
        return NULL;
    _exit(sts);
    /*NOTREACHED*/
    Py_FatalError("_exit() called from Python code didn't exit!");
    return NULL; /* Make gcc -Wall happy */
}
PyDoc_STRVAR(morphos_getegid__doc__,
"getegid() -> egid\n\n\
Return the current process's effective group id.");

static PyObject *
morphos_getegid(PyObject *self, PyObject *noargs)
{
	return PyLong_FromLong((long)getegid());
}
PyDoc_STRVAR(morphos_geteuid__doc__,
"geteuid() -> euid\n\n\
Return the current process's effective user id.");

static PyObject *
morphos_geteuid(PyObject *self, PyObject *noargs)
{
	return PyLong_FromLong((long)geteuid());
}
PyDoc_STRVAR(morphos_getgid__doc__,
"getgid() -> gid\n\n\
Return the current process's group id.");

static PyObject *
morphos_getgid(PyObject *self, PyObject *noargs)
{
	return PyLong_FromLong((long)getgid());
}
PyDoc_STRVAR(morphos_getpid__doc__,
"getpid() -> pid\n\n\
Return the current process id");

static PyObject *
morphos_getpid(PyObject *self, PyObject *noargs)
{
    long pid;

    if (!NewGetTaskAttrsA(NULL, &pid, sizeof(long), TASKINFOTYPE_PID, 0))
        pid = (long)FindTask(NULL);

    return PyLong_FromLong(pid);
}
PyDoc_STRVAR(morphos_getgroups__doc__,
"getgroups() -> list of group IDs\n\n\
Return list of supplemental group IDs for the process.");

static PyObject *
morphos_getgroups(PyObject *self, PyObject *noargs)
{
    PyObject *result = NULL;

#ifdef NGROUPS_MAX
#define MAX_GROUPS NGROUPS_MAX
#else
        /* defined to be 16 on Solaris7, so this should be a small number */
#define MAX_GROUPS 64
#endif
    gid_t grouplist[MAX_GROUPS];
    int n;

    n = getgroups(MAX_GROUPS, grouplist);
    if (n < 0)
        morphos_error();
    else {
        result = PyList_New(n);
        if (result != NULL) {
            int i;
            for (i = 0; i < n; ++i) {
                PyObject *o = PyLong_FromLong((long)grouplist[i]);
                if (o == NULL) {
                    Py_DECREF(result);
                    result = NULL;
                    break;
                }
                PyList_SET_ITEM(result, i, o);
            }
        }
    }

    return result;
}
PyDoc_STRVAR(morphos_getpgrp__doc__,
"getpgrp() -> pgrp\n\n\
Return the current process group id.");

static PyObject *
morphos_getpgrp(PyObject *self, PyObject *noargs)
{
#ifdef GETPGRP_HAVE_ARG
    return PyLong_FromPid(getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
    return PyLong_FromPid(getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}
PyDoc_STRVAR(morphos_getlogin__doc__,
"getlogin() -> string\n\n\
Return the actual login name.");

static PyObject *
morphos_getlogin(PyObject *self, PyObject *noargs)
{
    PyObject *result = NULL;
    char *name;
    int old_errno = errno;

    errno = 0;
    name = getlogin();
    if (name == NULL) {
        if (errno)
            morphos_error();
        else
            PyErr_SetString(PyExc_OSError,
                            "unable to determine login name");
    }
    else
        result = PyUnicode_DecodeLatin1(name, strlen(name), "surrogateescape");
    errno = old_errno;

    return result;
}
PyDoc_STRVAR(morphos_getuid__doc__,
"getuid() -> uid\n\n\
Return the current process's user id.");

static PyObject *
morphos_getuid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromLong((long)getuid());
}
//-

#ifdef HAVE_PLOCK
#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif
//+ morphos_plock()
PyDoc_STRVAR(morphos_plock__doc__,
"plock(op)\n\n\
Lock program segments into memory.");

static PyObject *
morphos_plock(PyObject *self, PyObject *args)
{
    int op;
    if (!PyArg_ParseTuple(args, "i:plock", &op))
        return NULL;
    if (plock(op) == -1)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}
//-
#endif

PyDoc_STRVAR(morphos_setuid__doc__,
"setuid(uid)\n\n\
Set the current process's user id.");

static PyObject *
morphos_setuid(PyObject *self, PyObject *args)
{
	long uid_arg;
	uid_t uid;
	if (!PyArg_ParseTuple(args, "l:setuid", &uid_arg))
		return NULL;
	uid = uid_arg;
	if (uid != uid_arg) {
		PyErr_SetString(PyExc_OverflowError, "user id too big");
		return NULL;
	}
	if (setuid(uid) < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(morphos_setreuid__doc__,
"setreuid(ruid, euid)\n\n\
Set the current process's real and effective user ids.");

static PyObject *
morphos_setreuid (PyObject *self, PyObject *args)
{
	long ruid_arg, euid_arg;
	uid_t ruid, euid;
	if (!PyArg_ParseTuple(args, "ll", &ruid_arg, &euid_arg))
		return NULL;
	ruid = ruid_arg;
	euid = euid_arg;
	if (euid != euid_arg || ruid != ruid_arg) {
		PyErr_SetString(PyExc_OverflowError, "user id too big");
		return NULL;
	}
	if (setreuid(ruid, euid) < 0) {
		return morphos_error();
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}
PyDoc_STRVAR(morphos_setregid__doc__,
"setregid(rgid, egid)\n\n\
Set the current process's real and effective group ids.");

static PyObject *
morphos_setregid (PyObject *self, PyObject *args)
{
	long rgid_arg, egid_arg;
	gid_t rgid, egid;
	if (!PyArg_ParseTuple(args, "ll", &rgid_arg, &egid_arg))
		return NULL;
	rgid = rgid_arg;
	egid = egid_arg;
	if (egid != egid_arg || rgid != rgid_arg) {
		PyErr_SetString(PyExc_OverflowError, "group id too big");
		return NULL;
	}
	if (setregid(rgid, egid) < 0) {
		return morphos_error();
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}
PyDoc_STRVAR(morphos_setgid__doc__,
"setgid(gid)\n\n\
Set the current process's group id.");

static PyObject *
morphos_setgid(PyObject *self, PyObject *args)
{
	long gid_arg;
	gid_t gid;
	if (!PyArg_ParseTuple(args, "l:setgid", &gid_arg))
		return NULL;
	gid = gid_arg;
	if (gid != gid_arg) {
		PyErr_SetString(PyExc_OverflowError, "group id too big");
		return NULL;
	}
	if (setgid(gid) < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(morphos_setgroups__doc__,
"setgroups(list)\n\n\
Set the groups of the current process to list.");

static PyObject *
morphos_setgroups(PyObject *self, PyObject *groups)
{
	int i, len;
        gid_t grouplist[MAX_GROUPS];

	if (!PySequence_Check(groups)) {
		PyErr_SetString(PyExc_TypeError, "setgroups argument must be a sequence");
		return NULL;
	}
	len = PySequence_Size(groups);
	if (len > MAX_GROUPS) {
		PyErr_SetString(PyExc_ValueError, "too many groups");
		return NULL;
	}
	for(i = 0; i < len; i++) {
		PyObject *elem;
		elem = PySequence_GetItem(groups, i);
		if (!elem)
			return NULL;
		if (!PyLong_Check(elem)) {
			PyErr_SetString(PyExc_TypeError,
					"groups must be integers");
			Py_DECREF(elem);
			return NULL;
		} else {
			unsigned long x = PyLong_AsUnsignedLong(elem);
			if (PyErr_Occurred()) {
				PyErr_SetString(PyExc_TypeError, 
						"group id too big");
				Py_DECREF(elem);
				return NULL;
			}
			grouplist[i] = x;
			/* read back the value to see if it fitted in gid_t */
			if (grouplist[i] != x) {
				PyErr_SetString(PyExc_TypeError,
						"group id too big");
				Py_DECREF(elem);
				return NULL;
			}
		}
		Py_DECREF(elem);
	}

	if (setgroups(len, grouplist) < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(morphos_lstat__doc__,
"lstat(path) -> stat result\n\n\
Like stat(path), but do not follow symbolic links.");

static PyObject *
morphos_lstat(PyObject *self, PyObject *args)
{
#ifdef HAVE_LSTAT
	return morphos_do_stat(self, args, "O&:lstat", lstat, NULL, NULL);
#else
	return morphos_do_stat(self, args, "O&:lstat", STAT, NULL, NULL);
#endif /* !HAVE_LSTAT */
}
#ifdef HAVE_READLINK
//+ morphos_readlink()
PyDoc_STRVAR(morphos_readlink__doc__,
"readlink(path) -> path\n\n\
Return a string representing the path to which the symbolic link points.");

static PyObject *
morphos_readlink(PyObject *self, PyObject *args)
{
	PyObject* v;
	char buf[MAXPATHLEN];
	PyObject *opath;
	char *path;
	int n;
	int arg_is_unicode = 0;

	if (!PyArg_ParseTuple(args, "O&:readlink", 
                          PyUnicode_FSConverter, &opath))
		return NULL;
	path = PyBytes_AsString(opath);
	v = PySequence_GetItem(args, 0);
	if (v == NULL) {
		Py_DECREF(opath);
		return NULL;
	}

	if (PyUnicode_Check(v)) {
		arg_is_unicode = 1;
	}
	Py_DECREF(v);

	Py_BEGIN_ALLOW_THREADS
	n = readlink(path, buf, (int) sizeof buf);
	Py_END_ALLOW_THREADS
	if (n < 0)
		return morphos_error_with_allocated_filename(opath);

	Py_DECREF(opath);
	v = PyBytes_FromStringAndSize(buf, n);
	if (arg_is_unicode) {
		PyObject *w;

		w = PyUnicode_FromEncodedObject(v,
				Py_FileSystemDefaultEncoding,
				"surrogateescape");
		if (w != NULL) {
			Py_DECREF(v);
			v = w;
		}
		else {
			v = NULL;
		}
	}
	return v;
}
//-
#endif /* HAVE_READLINK */

PyDoc_STRVAR(morphos_symlink__doc__,
"symlink(src, dst)\n\n\
Create a symbolic link pointing to src named dst.");

static PyObject *
morphos_symlink(PyObject *self, PyObject *args)
{
    char *path1 = NULL, *path2 = NULL;
    int res = -1;

    if (!PyArg_ParseTuple(args, "O&O&:symlink",
                          PyUnicode_FSConverter, &path1,
                          PyUnicode_FSConverter, &path2))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = !MakeLink(path2, (LONG)path1, TRUE);
    if (res)
        __seterrno();
    Py_END_ALLOW_THREADS

    PyMem_Free(path1);
    PyMem_Free(path2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#ifdef HAVE_TIMES
//+ morphos_times()
PyDoc_STRVAR(morphos_times__doc__,
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\n\
Return a tuple of floating point numbers indicating process times.");

#define NEED_TICKS_PER_SECOND
static long ticks_per_second = -1;
static PyObject *
morphos_times(PyObject *self, PyObject *noargs)
{
	struct tms t;
	clock_t c;
	errno = 0;
	c = times(&t);
	if (c == (clock_t) -1)
		return morphos_error();
	return Py_BuildValue("ddddd",
			     (double)t.tms_utime / ticks_per_second,
			     (double)t.tms_stime / ticks_per_second,
			     (double)t.tms_cutime / ticks_per_second,
			     (double)t.tms_cstime / ticks_per_second,
			     (double)c / ticks_per_second);
}
//-
#endif /* HAVE_TIMES */

PyDoc_STRVAR(morphos_setsid__doc__,
"setsid()\n\n\
Call the system call setsid().");

static PyObject *
morphos_setsid(PyObject *self, PyObject *noargs)
{
	if (setsid() < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
/* Functions acting on file descriptors */

PyDoc_STRVAR(morphos_GetOSFileHandle__doc__,
"getosfh(fd) -> os fh\n\n\
Return AmigaDOS specific filehandle (BPTR) from given file descriptor.");

extern unsigned long __get_handle(int fd);

static PyObject *
morphos_GetOSFileHandle(PyObject *self, PyObject *args)
{
    BPTR fh;
    int fd;

    if (!PyArg_ParseTuple(args, "i", &fd))
        return NULL;
        
    if (!_PyVerify_fd(fd))
		return morphos_error();

    Py_BEGIN_ALLOW_THREADS
    fh = __get_handle(fd);
    Py_END_ALLOW_THREADS
    if (NULL == fh)
        return morphos_error();
    return PyLong_FromLong(fh);
}
PyDoc_STRVAR(morphos_open__doc__,
"open(filename, flag [, mode=0777]) -> fd\n\n\
Open a file (for low level IO).");

static PyObject *
morphos_open(PyObject *self, PyObject *args)
{
	PyObject *ofile;
	char *file;
	int flag;
	int mode = 0777;
	int fd;

	if (!PyArg_ParseTuple(args, "O&i|i",
	                      PyUnicode_FSConverter, &ofile,
	                      &flag, &mode))
		return NULL;
	file = PyBytes_AsString(ofile);
	Py_BEGIN_ALLOW_THREADS
	fd = open(file, flag, mode);
	Py_END_ALLOW_THREADS
	if (fd < 0)
		return morphos_error_with_allocated_filename(ofile);
	Py_DECREF(ofile);
	return PyLong_FromLong((long)fd);
}
PyDoc_STRVAR(morphos_close__doc__,
"close(fd)\n\n\
Close a file descriptor (for low level IO).");

static PyObject *
morphos_close(PyObject *self, PyObject *args)
{
	int fd, res;
	if (!PyArg_ParseTuple(args, "i:close", &fd))
		return NULL;
	if (!_PyVerify_fd(fd))
		return morphos_error();
	Py_BEGIN_ALLOW_THREADS
	res = close(fd);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(morphos_closerange__doc__, 
"closerange(fd_low, fd_high)\n\n\
Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

static PyObject *
morphos_closerange(PyObject *self, PyObject *args)
{
	int fd_from, fd_to, i;
	if (!PyArg_ParseTuple(args, "ii:closerange", &fd_from, &fd_to))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	for (i = fd_from; i < fd_to; i++)
		if (_PyVerify_fd(i))
			close(i);
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}
PyDoc_STRVAR(morphos_dup__doc__,
"dup(fd) -> fd2\n\n\
Return a duplicate of a file descriptor.");

static PyObject *
morphos_dup(PyObject *self, PyObject *args)
{
	int fd;
	if (!PyArg_ParseTuple(args, "i:dup", &fd))
		return NULL;
	if (!_PyVerify_fd(fd))
		return morphos_error();
	Py_BEGIN_ALLOW_THREADS
	fd = dup(fd);
	Py_END_ALLOW_THREADS
	if (fd < 0)
		return morphos_error();
	return PyLong_FromLong((long)fd);
}
PyDoc_STRVAR(morphos_dup2__doc__,
"dup2(old_fd, new_fd)\n\n\
Duplicate file descriptor.");

static PyObject *
morphos_dup2(PyObject *self, PyObject *args)
{
	int fd, fd2, res;
	if (!PyArg_ParseTuple(args, "ii:dup2", &fd, &fd2))
		return NULL;
	if (!_PyVerify_fd_dup2(fd, fd2))
		return morphos_error();
	Py_BEGIN_ALLOW_THREADS
	res = dup2(fd, fd2);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
PyDoc_STRVAR(morphos_lseek__doc__,
"lseek(fd, pos, how) -> newpos\n\n\
Set the current position of a file descriptor.");

static PyObject *
morphos_lseek(PyObject *self, PyObject *args)
{
    int fd, how;
    off_t pos, res;
    PyObject *posobj;
    if (!PyArg_ParseTuple(args, "iOi:lseek", &fd, &posobj, &how))
        return NULL;
#ifdef SEEK_SET
    /* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
    switch (how) {
    case 0: how = SEEK_SET; break;
    case 1: how = SEEK_CUR; break;
    case 2: how = SEEK_END; break;
    }
#endif /* SEEK_END */

    if (!_PyVerify_fd(fd))
		return morphos_error();

#if !defined(HAVE_LARGEFILE_SUPPORT)
    pos = PyLong_AsLong(posobj);
#else
    pos = PyLong_Check(posobj) ?
        PyLong_AsLongLong(posobj) : PyInt_AsLong(posobj);
#endif
    if (PyErr_Occurred())
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = lseek(fd, pos, how);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();

#if !defined(HAVE_LARGEFILE_SUPPORT)
    return PyLong_FromLong(res);
#else
    return PyLong_FromLongLong(res);
#endif
}
PyDoc_STRVAR(morphos_read__doc__,
"read(fd, buffersize) -> string\n\n\
Read a file descriptor.");

static PyObject *
morphos_read(PyObject *self, PyObject *args)
{
	int fd, size;
    Py_ssize_t n;
	PyObject *buffer;
	if (!PyArg_ParseTuple(args, "ii:read", &fd, &size))
		return NULL;
	if (size < 0) {
		errno = EINVAL;
		return morphos_error();
	}
	buffer = PyBytes_FromStringAndSize((char *)NULL, size);
	if (buffer == NULL)
		return NULL;
	if (!_PyVerify_fd(fd))
		return morphos_error();
	Py_BEGIN_ALLOW_THREADS
	n = read(fd, PyBytes_AS_STRING(buffer), size);
	Py_END_ALLOW_THREADS
	if (n < 0) {
		Py_DECREF(buffer);
		return morphos_error();
	}
	if (n != size)
		_PyBytes_Resize(&buffer, n);
	return buffer;
}
PyDoc_STRVAR(morphos_write__doc__,
"write(fd, string) -> byteswritten\n\n\
Write a string to a file descriptor.");

static PyObject *
morphos_write(PyObject *self, PyObject *args)
{
    Py_buffer pbuf;
    int fd;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "iy*:write", &fd, &pbuf))
        return NULL;
	if (!_PyVerify_fd(fd))
		return morphos_error();
    Py_BEGIN_ALLOW_THREADS
    size = write(fd, pbuf.buf, (size_t)pbuf.len);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&pbuf);
    if (size < 0)
        return morphos_error();
    return PyLong_FromLong(size);
}
PyDoc_STRVAR(morphos_fstat__doc__,
"fstat(fd) -> stat result\n\n\
Like stat(), but for an open file descriptor.");

static PyObject *
morphos_fstat(PyObject *self, PyObject *args)
{
    int fd;
    STRUCT_STAT st;
    int res;

    if (!PyArg_ParseTuple(args, "i:fstat", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
		return morphos_error();
    Py_BEGIN_ALLOW_THREADS
    res = FSTAT(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return morphos_error();

    return _pystat_fromstructstat(&st);
}
PyDoc_STRVAR(morphos_isatty__doc__,
"isatty(fd) -> bool\n\n\
Return True if the file descriptor 'fd' is an open file descriptor\n\
connected to the slave end of a terminal.");

static PyObject *
morphos_isatty(PyObject *self, PyObject *args)
{
	int fd;
	if (!PyArg_ParseTuple(args, "i:isatty", &fd))
		return NULL;
	if (!_PyVerify_fd(fd))
		return PyBool_FromLong(0);
	return PyBool_FromLong(isatty(fd));
}
PyDoc_STRVAR(morphos_pipe__doc__,
"pipe() -> (read_end, write_end)\n\n\
Create a pipe.");

static PyObject *
morphos_pipe(PyObject *self, PyObject *noargs)
{
	int fd;
    char buf[24];

    snprintf(buf, sizeof(buf), "PIPE:py3pipe_%08x", GetUniqueID());

	Py_BEGIN_ALLOW_THREADS
	fd = open(buf, O_RDWR);
	Py_END_ALLOW_THREADS
	
    if (fd < 0)
		return morphos_error();
	
    return Py_BuildValue("(ii)", fd, fd);
}

#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
//+ morphos_mknod
PyDoc_STRVAR(morphos_mknod__doc__,
"mknod(filename [, mode=0600, device])\n\n\
Create a filesystem node (file, device special file or named pipe)\n\
named filename. mode specifies both the permissions to use and the\n\
type of node to be created, being combined (bitwise OR) with one of\n\
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO. For S_IFCHR and S_IFBLK,\n\
device defines the newly created device special file (probably using\n\
os.makedev()), otherwise it is ignored.");


static PyObject *
morphos_mknod(PyObject *self, PyObject *args)
{
	char *filename;
	int mode = 0600;
	int device = 0;
	int res;
	if (!PyArg_ParseTuple(args, "s|ii:mknod", &filename, &mode, &device))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	res = mknod(filename, mode, device);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
//-
#endif

#ifdef HAVE_DEVICE_MACROS
//+ morphos_major()
PyDoc_STRVAR(morphos_major__doc__,
"major(device) -> major number\n\
Extracts a device major number from a raw device number.");

static PyObject *
morphos_major(PyObject *self, PyObject *args)
{
	int device;
	if (!PyArg_ParseTuple(args, "i:major", &device))
		return NULL;
	return PyLong_FromLong((long)major(device));
}
//-
//+ morphos_minor()
PyDoc_STRVAR(morphos_minor__doc__,
"minor(device) -> minor number\n\
Extracts a device minor number from a raw device number.");

static PyObject *
morphos_minor(PyObject *self, PyObject *args)
{
	int device;
	if (!PyArg_ParseTuple(args, "i:minor", &device))
		return NULL;
	return PyLong_FromLong((long)minor(device));
}
//-
//+ morphos_makedev()
PyDoc_STRVAR(morphos_makedev__doc__,
"makedev(major, minor) -> device number\n\
Composes a raw device number from the major and minor device numbers.");

static PyObject *
morphos_makedev(PyObject *self, PyObject *args)
{
	int major, minor;
	if (!PyArg_ParseTuple(args, "ii:makedev", &major, &minor))
		return NULL;
	return PyLong_FromLong((long)makedev(major, minor));
}
//-
#endif /* device macros */

PyDoc_STRVAR(morphos_ftruncate__doc__,
"ftruncate(fd, length)\n\n\
Truncate a file to a specified length.");

static PyObject *
morphos_ftruncate(PyObject *self, PyObject *args)
{
	int fd;
	off_t length;
	int res;
	PyObject *lenobj;

	if (!PyArg_ParseTuple(args, "iO:ftruncate", &fd, &lenobj))
		return NULL;
		
    if (!_PyVerify_fd(fd))
		return morphos_error();

#if !defined(HAVE_LARGEFILE_SUPPORT)
	length = PyLong_AsLong(lenobj);
#else
	length = PyLong_Check(lenobj) ?
		PyLong_AsLongLong(lenobj) : PyLong_AsLong(lenobj);
#endif
	if (PyErr_Occurred())
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	res = ftruncate(fd, length);
	Py_END_ALLOW_THREADS
	if (res < 0)
		return morphos_error();
	Py_INCREF(Py_None);
	return Py_None;
}
#ifdef HAVE_PUTENV
//+ morphos_putenv
PyDoc_STRVAR(morphos_putenv__doc__,
"putenv(key, value)\n\n\
Change or add an environment variable.");

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *morphos_putenv_garbage;

static PyObject *
morphos_putenv(PyObject *self, PyObject *args)
{

	PyObject *os1, *os2;
    char *s1, *s2;
    char *newenv;
	PyObject *newstr;
	size_t len;

	if (!PyArg_ParseTuple(args,
			      "O&O&:putenv",
			      PyUnicode_FSConverter, &os1, 
			      PyUnicode_FSConverter, &os2))
		return NULL;
	s1 = PyBytes_AsString(os1);
	s2 = PyBytes_AsString(os2);

	/* XXX This can leak memory -- not easy to fix :-( */
	/* len includes space for a trailing \0; the size arg to
	   PyBytes_FromStringAndSize does not count that */
	len = strlen(s1) + strlen(s2) + 2;
	newstr = PyBytes_FromStringAndSize(NULL, (int)len - 1);
	if (newstr == NULL)
		return PyErr_NoMemory();
	newenv = PyBytes_AS_STRING(newstr);
	PyOS_snprintf(newenv, len, "%s=%s", s1, s2);
	if (putenv(newenv)) {
        Py_DECREF(newstr);
		Py_DECREF(os1);
		Py_DECREF(os2);
        morphos_error();
        return NULL;
	}
	/* Install the first arg and newstr in morphos_putenv_garbage;
	 * this will cause previous value to be collected.  This has to
	 * happen after the real putenv() call because the old value
	 * was still accessible until then. */
	if (PyDict_SetItem(morphos_putenv_garbage,
                       PyTuple_GET_ITEM(args, 0), newstr)) {
		/* really not much we can do; just leak */
		PyErr_Clear();
	}
	else {
		Py_DECREF(newstr);
	}

	Py_DECREF(os1);
	Py_DECREF(os2);
	Py_INCREF(Py_None);
    return Py_None;
}
//-
#endif /* putenv */

#ifdef HAVE_UNSETENV
//+ morphos_unsetenv()
PyDoc_STRVAR(morphos_unsetenv__doc__,
"unsetenv(key)\n\n\
Delete an environment variable.");

static PyObject *
morphos_unsetenv(PyObject *self, PyObject *args)
{
        char *s1;

	if (!PyArg_ParseTuple(args, "s:unsetenv", &s1))
		return NULL;

	unsetenv(s1);

	/* Remove the key from morphos_putenv_garbage;
	 * this will cause it to be collected.  This has to
	 * happen after the real unsetenv() call because the
	 * old value was still accessible until then.
	 */
	if (PyDict_DelItem(morphos_putenv_garbage,
		PyTuple_GET_ITEM(args, 0))) {
		/* really not much we can do; just leak */
		PyErr_Clear();
	}

	Py_INCREF(Py_None);
	return Py_None;
}
//-
#endif /* unsetenv */

PyDoc_STRVAR(morphos_strerror__doc__,
"strerror(code) -> string\n\n\
Translate an error code to a message string.");

static PyObject *
morphos_strerror(PyObject *self, PyObject *args)
{
	int code;
	char *message;
	if (!PyArg_ParseTuple(args, "i:strerror", &code))
		return NULL;
	message = strerror(code);
	if (message == NULL) {
		PyErr_SetString(PyExc_ValueError,
				"strerror() argument out of range");
		return NULL;
	}
	return PyUnicode_FromString(message);
}
#if 0 /* TODO: to be moved !*/
//+ morphos_popen()
PyDoc_STRVAR(morphos_popen__doc__,
"popen(command [, mode='r' [, bufsize]]) -> pipe\n\n\
Open a pipe to/from a command returning a file object.");

static PyObject *
morphos_popen(PyObject *self, PyObject *args)
{
    char *name;
    char *mode = "r";
    int bufsize = -1;
    FILE *fp;
    PyObject *f;

    if (!PyArg_ParseTuple(args, "s|si:popen", &name, &mode, &bufsize))
        return NULL;

    /* Strip mode of binary or text modifiers */
    if ((strcmp(mode, "rb") == 0) || (strcmp(mode, "rt") == 0))
        mode = "r";
    else if ((strcmp(mode, "wb") == 0) || (strcmp(mode, "wt") == 0))
        mode = "w";

    Py_BEGIN_ALLOW_THREADS
    fp = popen(name, mode);
    Py_END_ALLOW_THREADS

    if (fp == NULL)
    {
        SetIoErr(errno);
        return morphos_error();
    }

    f = PyFile_FromFile(fp, name, mode, pclose);
    if (f != NULL)
        PyFile_SetBufSize(f, bufsize);

    return f;
}
//-
#endif

#if defined(HAVE_FSTATVFS)
#include <sys/statvfs.h>

//+ _pystatvfs_fromstructstatvfs()
static PyObject*
_pystatvfs_fromstructstatvfs(struct statvfs st) {
        PyObject *v = PyStructSequence_New(&StatVFSResultType);
    if (v == NULL)
        return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
        PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
        PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
        PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long) st.f_blocks));
        PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long) st.f_bfree));
        PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long) st.f_bavail));
        PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long) st.f_files));
        PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong((long) st.f_ffree));
        PyStructSequence_SET_ITEM(v, 7, PyInt_FromLong((long) st.f_favail));
        PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
        PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#else
        PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
        PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
        PyStructSequence_SET_ITEM(v, 2,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_blocks));
        PyStructSequence_SET_ITEM(v, 3,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_bfree));
        PyStructSequence_SET_ITEM(v, 4,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_bavail));
        PyStructSequence_SET_ITEM(v, 5,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_files));
        PyStructSequence_SET_ITEM(v, 6,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_ffree));
        PyStructSequence_SET_ITEM(v, 7,
                   PyLong_FromLongLong((PY_LONG_LONG) st.f_favail));
        PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
        PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#endif

        return v;
}
//-
//+ morphos_fstatvfs()
PyDoc_STRVAR(morphos_fstatvfs__doc__,
"fstatvfs(fd) -> statvfs result\n\n\
Perform an fstatvfs system call on the given fd.");

static PyObject *
morphos_fstatvfs(PyObject *self, PyObject *args)
{
    int fd, res;
    struct statvfs st;

    if (!PyArg_ParseTuple(args, "i:fstatvfs", &fd))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fstatvfs(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return morphos_error();

        return _pystatvfs_fromstructstatvfs(st);
}
//-
#endif /* HAVE_FSTATVFS */

#if defined(HAVE_STATVFS)
#include <sys/statvfs.h>
//+ morphos_statvfs()
PyDoc_STRVAR(morphos_statvfs__doc__,
"statvfs(path) -> statvfs result\n\n\
Perform a statvfs system call on the given path.");

static PyObject *
morphos_statvfs(PyObject *self, PyObject *args)
{
    char *path;
    int res;
    struct statvfs st;
    if (!PyArg_ParseTuple(args, "s:statvfs", &path))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = statvfs(path, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return morphos_error_with_filename(path);

        return _pystatvfs_fromstructstatvfs(st);
}
//-
#endif /* HAVE_STATVFS */

PyDoc_STRVAR(morphos_abort__doc__,
"abort() -> does not return!\n\n\
Abort the interpreter immediately.  This 'dumps core' or otherwise fails\n\
in the hardest way possible on the hosting operating system.");

static PyObject *
morphos_abort(PyObject *self, PyObject *noargs)
{
    abort();
    /*NOTREACHED*/
    Py_FatalError("abort() called from Python code didn't abort!");
    return NULL;
}


#ifdef HAVE_GETLOADAVG
//+ morphos_getloadavg()
PyDoc_STRVAR(morphos_getloadavg__doc__,
"getloadavg() -> (float, float, float)\n\n\
Return the number of processes in the system run queue averaged over\n\
the last 1, 5, and 15 minutes or raises OSError if the load average\n\
was unobtainable");

static PyObject *
morphos_getloadavg(PyObject *self, PyObject *noargs)
{
    double loadavg[3];
    if (getloadavg(loadavg, 3)!=3) {
        PyErr_SetString(PyExc_OSError, "Load averages are unobtainable");
        return NULL;
    } else
        return Py_BuildValue("ddd", loadavg[0], loadavg[1], loadavg[2]);
}
//-
#endif

PyDoc_STRVAR(morphos_urandom__doc__,
"urandom(n) -> str\n\n\
Return a string of n random bytes suitable for cryptographic use.");

static PyObject*
morphos_urandom(PyObject *self, PyObject *args)
{
    pPyMorphOS_ThreadData_t td = GET_THREAD_DATA_PTR();
    struct Library * TimerBase = GET_THREAD_DATA(td, TimerBase);
    PyObject* result;
    char * s;
    ULONG i, count, seed;
    struct timeval tv;

    GetSysTime(&tv);
    seed = tv.tv_micro;

    /* Read arguments */
    if (!PyArg_ParseTuple(args, "i:urandom", &count))
        return NULL;
    if (count < 0)
        return PyErr_Format(PyExc_ValueError,
                    "negative argument not allowed");

    /* Allocate bytes */
    result = PyBytes_FromStringAndSize(NULL, count);
    if (result != NULL) {
        /* Get random data */
        s = PyBytes_AS_STRING(result);
        for (i=count >> 2; i; i--) {
            ULONG *p = (ULONG *)s;

            seed = FastRand(seed);
            *(p++) = seed;
        }

        for (; count % 4; count--) {
            seed = FastRand(seed);
            *(s++) = seed;
        }

        *s = '\0';
    }

    return result;
}
PyDoc_STRVAR(device_encoding__doc__,
"device_encoding(fd) -> str\n\n\
Return a string describing the encoding of the device\n\
if the output is a terminal; else return None.");

static PyObject *
device_encoding(PyObject *self, PyObject *args)
{
    static const char default_encoding[] = "latin-1";
	int fd;

	if (!PyArg_ParseTuple(args, "i:device_encoding", &fd))
		return NULL;
	if (!_PyVerify_fd(fd) || !isatty(fd)) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    return PyUnicode_DecodeLatin1(default_encoding,
                                  strlen(default_encoding),
                                  NULL);
}
static PyMethodDef morphos_methods[] = {
	{"access",	morphos_access, METH_VARARGS, morphos_access__doc__},
	{"chdir",	morphos_chdir, METH_VARARGS, morphos_chdir__doc__},
#ifdef HAVE_CHFLAGS
	{"chflags",	morphos_chflags, METH_VARARGS, morphos_chflags__doc__},
#endif /* HAVE_CHFLAGS */
	{"chmod",	morphos_chmod, METH_VARARGS, morphos_chmod__doc__},
	{"chown",	morphos_chown, METH_VARARGS, morphos_chown__doc__},
#ifdef HAVE_LCHMOD
	{"lchmod",	morphos_lchmod, METH_VARARGS, morphos_lchmod__doc__},
#endif /* HAVE_LCHMOD */
#ifdef HAVE_LCHFLAGS
	{"lchflags",	morphos_lchflags, METH_VARARGS, morphos_lchflags__doc__},
#endif /* HAVE_LCHFLAGS */
#ifdef HAVE_LCHOWN
	{"lchown",	morphos_lchown, METH_VARARGS, morphos_lchown__doc__},
#endif /* HAVE_LCHOWN */
#ifdef HAVE_CTERMID
	{"ctermid",	morphos_ctermid, METH_NOARGS, morphos_ctermid__doc__},
#endif
	{"getcwd",	(PyCFunction)morphos_getcwd_unicode, METH_NOARGS, morphos_getcwd__doc__},
	{"getcwdb",	(PyCFunction)morphos_getcwd_bytes, METH_NOARGS, morphos_getcwdb__doc__},
	{"link",	morphos_link, METH_VARARGS, morphos_link__doc__},
	{"listdir",	morphos_listdir, METH_VARARGS, morphos_listdir__doc__},
	{"lstat",	morphos_lstat, METH_VARARGS, morphos_lstat__doc__},
	{"mkdir",	morphos_mkdir, METH_VARARGS, morphos_mkdir__doc__},
	{"nice",	morphos_nice, METH_VARARGS, morphos_nice__doc__},
#ifdef HAVE_READLINK
	{"readlink",	morphos_readlink, METH_VARARGS, morphos_readlink__doc__},
#endif /* HAVE_READLINK */
	{"rename",	morphos_rename, METH_VARARGS, morphos_rename__doc__},
	{"rmdir",	morphos_rmdir, METH_VARARGS, morphos_rmdir__doc__},
	{"stat",	morphos_stat, METH_VARARGS, morphos_stat__doc__},
	{"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
	{"symlink",	morphos_symlink, METH_VARARGS, morphos_symlink__doc__},
	{"system",	morphos_system, METH_VARARGS, morphos_system__doc__},
	{"umask",	morphos_umask, METH_VARARGS, morphos_umask__doc__},
#ifdef HAVE_UNAME
	{"uname",	morphos_uname, METH_NOARGS, morphos_uname__doc__},
#endif /* HAVE_UNAME */
	{"unlink",	morphos_unlink, METH_VARARGS, morphos_unlink__doc__},
	{"remove",	morphos_unlink, METH_VARARGS, morphos_remove__doc__},
	{"utime",	morphos_utime, METH_VARARGS, morphos_utime__doc__},
#ifdef HAVE_TIMES
	{"times",	morphos_times, METH_NOARGS, morphos_times__doc__},
#endif /* HAVE_TIMES */
	{"_exit",	morphos__exit, METH_VARARGS, morphos__exit__doc__},
	{"getegid",	morphos_getegid, METH_NOARGS, morphos_getegid__doc__},
	{"geteuid",	morphos_geteuid, METH_NOARGS, morphos_geteuid__doc__},
	{"getgid",	morphos_getgid, METH_NOARGS, morphos_getgid__doc__},
	{"getgroups",	morphos_getgroups, METH_NOARGS, morphos_getgroups__doc__},
	{"getpid",	morphos_getpid, METH_NOARGS, morphos_getpid__doc__},
	{"getpgrp",	morphos_getpgrp, METH_NOARGS, morphos_getpgrp__doc__},
	{"getuid",	morphos_getuid, METH_NOARGS, morphos_getuid__doc__},
	{"getlogin",	morphos_getlogin, METH_NOARGS, morphos_getlogin__doc__},
#ifdef HAVE_PLOCK
	{"plock",	morphos_plock, METH_VARARGS, morphos_plock__doc__},
#endif /* HAVE_PLOCK */
	{"setuid",	morphos_setuid, METH_VARARGS, morphos_setuid__doc__},
	{"setreuid",	morphos_setreuid, METH_VARARGS, morphos_setreuid__doc__},
	{"setregid",	morphos_setregid,	METH_VARARGS, morphos_setregid__doc__},
	{"setgid",	morphos_setgid, METH_VARARGS, morphos_setgid__doc__},
	{"setgroups",	morphos_setgroups, METH_O, morphos_setgroups__doc__},
	{"setsid",	morphos_setsid, METH_NOARGS, morphos_setsid__doc__},
#ifdef HAVE_TCGETPGRP
	{"tcgetpgrp",	morphos_tcgetpgrp, METH_VARARGS, morphos_tcgetpgrp__doc__},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
	{"tcsetpgrp",	morphos_tcsetpgrp, METH_VARARGS, morphos_tcsetpgrp__doc__},
#endif /* HAVE_TCSETPGRP */
	{"open",	morphos_open, METH_VARARGS, morphos_open__doc__},
	{"close",	morphos_close, METH_VARARGS, morphos_close__doc__},
	{"closerange",	morphos_closerange, METH_VARARGS, morphos_closerange__doc__},
	{"device_encoding", device_encoding, METH_VARARGS, device_encoding__doc__},
	{"dup",		morphos_dup, METH_VARARGS, morphos_dup__doc__},
	{"dup2",	morphos_dup2, METH_VARARGS, morphos_dup2__doc__},
	{"lseek",	morphos_lseek, METH_VARARGS, morphos_lseek__doc__},
	{"read",	morphos_read, METH_VARARGS, morphos_read__doc__},
	{"write",	morphos_write, METH_VARARGS, morphos_write__doc__},
	{"fstat",	morphos_fstat, METH_VARARGS, morphos_fstat__doc__},
	{"isatty",	morphos_isatty, METH_VARARGS, morphos_isatty__doc__},
	{"pipe",	morphos_pipe, METH_NOARGS, morphos_pipe__doc__},
#ifdef HAVE_MKFIFO
	{"mkfifo",	morphos_mkfifo, METH_VARARGS, morphos_mkfifo__doc__},
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
	{"mknod",	morphos_mknod, METH_VARARGS, morphos_mknod__doc__},
#endif
#ifdef HAVE_DEVICE_MACROS
	{"major",	morphos_major, METH_VARARGS, morphos_major__doc__},
	{"minor",	morphos_minor, METH_VARARGS, morphos_minor__doc__},
	{"makedev",	morphos_makedev, METH_VARARGS, morphos_makedev__doc__},
#endif
	{"ftruncate",	morphos_ftruncate, METH_VARARGS, morphos_ftruncate__doc__},
#ifdef HAVE_PUTENV
	{"putenv",	morphos_putenv, METH_VARARGS, morphos_putenv__doc__},
#endif
#ifdef HAVE_UNSETENV
	{"unsetenv",	morphos_unsetenv, METH_VARARGS, morphos_unsetenv__doc__},
#endif
	{"strerror",	morphos_strerror, METH_VARARGS, morphos_strerror__doc__},
#ifdef HAVE_FSYNC
	{"fsync",       morphos_fsync, METH_O, morphos_fsync__doc__},
#endif
#ifdef HAVE_FDATASYNC
	{"fdatasync",   morphos_fdatasync,  METH_O, morphos_fdatasync__doc__},
#endif
#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
	{"fstatvfs",	morphos_fstatvfs, METH_VARARGS, morphos_fstatvfs__doc__},
#endif
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
	{"statvfs",	morphos_statvfs, METH_VARARGS, morphos_statvfs__doc__},
#endif
	{"abort",	morphos_abort, METH_NOARGS, morphos_abort__doc__},
#ifdef HAVE_GETLOADAVG
	{"getloadavg",	morphos_getloadavg, METH_NOARGS, morphos_getloadavg__doc__},
#endif

    /* Morphos Specifics */
    {"urandom", morphos_urandom, METH_VARARGS, morphos_urandom__doc__},
    {"getosfh", morphos_GetOSFileHandle, METH_VARARGS, morphos_GetOSFileHandle__doc__},
	
    {NULL,		NULL}		 /* Sentinel */
};
static int
ins(PyObject *module, char *symbol, long value)
{
    return PyModule_AddIntConstant(module, symbol, value);
}
static int
all_ins(PyObject *d)
{
#ifdef F_OK
        if (ins(d, "F_OK", (long)F_OK)) return -1;
#endif
#ifdef R_OK
        if (ins(d, "R_OK", (long)R_OK)) return -1;
#endif
#ifdef W_OK
        if (ins(d, "W_OK", (long)W_OK)) return -1;
#endif
#ifdef X_OK
        if (ins(d, "X_OK", (long)X_OK)) return -1;
#endif
#ifdef NGROUPS_MAX
        if (ins(d, "NGROUPS_MAX", (long)NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
        if (ins(d, "TMP_MAX", (long)TMP_MAX)) return -1;
#endif
#ifdef O_RDONLY
        if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
        if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
        if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
        if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
        if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
        if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
        if (ins(d, "O_DSYNC", (long)O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
        if (ins(d, "O_RSYNC", (long)O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
        if (ins(d, "O_SYNC", (long)O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
        if (ins(d, "O_NOCTTY", (long)O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
        if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
#endif
#ifdef O_EXCL
        if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
        if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;
#endif
#ifdef O_BINARY
        if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
        if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif
#ifdef O_LARGEFILE
        if (ins(d, "O_LARGEFILE", (long)O_LARGEFILE)) return -1;
#endif
#ifdef O_SHLOCK
        if (ins(d, "O_SHLOCK", (long)O_SHLOCK)) return -1;
#endif
#ifdef O_EXLOCK
        if (ins(d, "O_EXLOCK", (long)O_EXLOCK)) return -1;
#endif

/* GNU extensions. */
#ifdef O_ASYNC
        /* Send a SIGIO signal whenever input or output 
           becomes available on file descriptor */
        if (ins(d, "O_ASYNC", (long)O_ASYNC)) return -1;
#endif
#ifdef O_DIRECT
        /* Direct disk access. */
        if (ins(d, "O_DIRECT", (long)O_DIRECT)) return -1;
#endif
#ifdef O_DIRECTORY
        /* Must be a directory.	 */
        if (ins(d, "O_DIRECTORY", (long)O_DIRECTORY)) return -1;
#endif
#ifdef O_NOFOLLOW
        /* Do not follow links.	 */
        if (ins(d, "O_NOFOLLOW", (long)O_NOFOLLOW)) return -1;
#endif
#ifdef O_NOATIME
	/* Do not update the access time. */
	if (ins(d, "O_NOATIME", (long)O_NOATIME)) return -1;
#endif

	/* These come from sysexits.h */
#ifdef EX_OK
	if (ins(d, "EX_OK", (long)EX_OK)) return -1;
#endif /* EX_OK */
#ifdef EX_USAGE
	if (ins(d, "EX_USAGE", (long)EX_USAGE)) return -1;
#endif /* EX_USAGE */
#ifdef EX_DATAERR
	if (ins(d, "EX_DATAERR", (long)EX_DATAERR)) return -1;
#endif /* EX_DATAERR */
#ifdef EX_NOINPUT
	if (ins(d, "EX_NOINPUT", (long)EX_NOINPUT)) return -1;
#endif /* EX_NOINPUT */
#ifdef EX_NOUSER
	if (ins(d, "EX_NOUSER", (long)EX_NOUSER)) return -1;
#endif /* EX_NOUSER */
#ifdef EX_NOHOST
	if (ins(d, "EX_NOHOST", (long)EX_NOHOST)) return -1;
#endif /* EX_NOHOST */
#ifdef EX_UNAVAILABLE
	if (ins(d, "EX_UNAVAILABLE", (long)EX_UNAVAILABLE)) return -1;
#endif /* EX_UNAVAILABLE */
#ifdef EX_SOFTWARE
	if (ins(d, "EX_SOFTWARE", (long)EX_SOFTWARE)) return -1;
#endif /* EX_SOFTWARE */
#ifdef EX_OSERR
	if (ins(d, "EX_OSERR", (long)EX_OSERR)) return -1;
#endif /* EX_OSERR */
#ifdef EX_OSFILE
	if (ins(d, "EX_OSFILE", (long)EX_OSFILE)) return -1;
#endif /* EX_OSFILE */
#ifdef EX_CANTCREAT
	if (ins(d, "EX_CANTCREAT", (long)EX_CANTCREAT)) return -1;
#endif /* EX_CANTCREAT */
#ifdef EX_IOERR
	if (ins(d, "EX_IOERR", (long)EX_IOERR)) return -1;
#endif /* EX_IOERR */
#ifdef EX_TEMPFAIL
	if (ins(d, "EX_TEMPFAIL", (long)EX_TEMPFAIL)) return -1;
#endif /* EX_TEMPFAIL */
#ifdef EX_PROTOCOL
	if (ins(d, "EX_PROTOCOL", (long)EX_PROTOCOL)) return -1;
#endif /* EX_PROTOCOL */
#ifdef EX_NOPERM
	if (ins(d, "EX_NOPERM", (long)EX_NOPERM)) return -1;
#endif /* EX_NOPERM */
#ifdef EX_CONFIG
	if (ins(d, "EX_CONFIG", (long)EX_CONFIG)) return -1;
#endif /* EX_CONFIG */
#ifdef EX_NOTFOUND
	if (ins(d, "EX_NOTFOUND", (long)EX_NOTFOUND)) return -1;
#endif /* EX_NOTFOUND */

        return 0;
}//-

#define INITFUNC PyInit_morphos
#define MODNAME "morphos"

static struct PyModuleDef morphosmodule = {
    PyModuleDef_HEAD_INIT,
    MODNAME,
    morphos__doc__,
    -1,
    morphos_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m, *dglobalv, *dlocalv, *dbothv, *daliases;

    m = PyModule_Create(&morphosmodule);
    if (m == NULL)
    	return NULL;

    /* Initialize environment dictionaries */
    if (!convertenviron(&dglobalv, &dlocalv, &dbothv, &daliases))
        return NULL;

    Py_XINCREF(dglobalv);
    Py_XINCREF(dlocalv);
    Py_XINCREF(dbothv);
    Py_XINCREF(daliases);

    if ((dbothv == NULL) || (PyModule_AddObject(m, "environ", dbothv) != 0))
        return NULL;

    if ((dglobalv == NULL) || (PyModule_AddObject(m, "globalvars", dbothv) != 0))
        return NULL;

    if ((dlocalv == NULL) || (PyModule_AddObject(m, "shellvars", dbothv) != 0))
        return NULL;

    if ((daliases == NULL) || (PyModule_AddObject(m, "shellaliases", dbothv) != 0))
        return NULL;

    Py_DECREF(dglobalv);
    Py_DECREF(dlocalv);
    Py_DECREF(dbothv);
    Py_DECREF(daliases);

    if (all_ins(m))
        return NULL;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

#ifdef HAVE_PUTENV
    if (morphos_putenv_garbage == NULL)
        morphos_putenv_garbage = PyDict_New();
#endif

	if (!initialized) {
		stat_result_desc.name = MODNAME ".stat_result";
		stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
		stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
		stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
		PyStructSequence_InitType(&StatResultType, &stat_result_desc);
		structseq_new = StatResultType.tp_new;
		StatResultType.tp_new = statresult_new;

		statvfs_result_desc.name = MODNAME ".statvfs_result";
		PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);

#ifdef NEED_TICKS_PER_SECOND
        ticks_per_second = CLOCKS_PER_SEC;
#endif
	}

    Py_INCREF((PyObject*) &StatResultType);
	PyModule_AddObject(m, "stat_result", (PyObject*) &StatResultType);
	Py_INCREF((PyObject*) &StatVFSResultType);
	PyModule_AddObject(m, "statvfs_result", (PyObject*) &StatVFSResultType);
    initialized = 1;

    return m;
}

