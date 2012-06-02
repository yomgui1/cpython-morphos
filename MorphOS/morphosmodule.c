/*******************************************************************************
 *** $Id$
 ***
 *** Morphos module implementation (adapted from original posixmodule.c)
 ***
 *******************************************************************************
 */

/*
** System Includes
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/usergroup.h>
#include <proto/alib.h>
#include <proto/timer.h> // used by morphos_urandom

#include <exec/lists.h>


/*
** Project Includes
*/

#include "Python.h"
#include "structseq.h"

#include "morphos.h"


/*
** Private Macros and Definitions
*/

PyDoc_STRVAR(morphos__doc__,
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the Morphos standard (ABox). \n\
Refer to the library manual and corresponding Morphos manual entries \n\
for more information on calls.");

#ifndef Py_USING_UNICODE
/* This is used in signatures of functions. */
#define Py_UNICODE void
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>       /* For WNOHANG */
#endif

#include <signal.h>
#include <fcntl.h>
#include <grp.h>
#include <sysexits.h>

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#include <sys/param.h>

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

/* Don't use the "_r" form if we don't need it (also, won't have a
   prototype for it, at least on Solaris -- maybe others as well?). */
#if defined(HAVE_CTERMID_R) && defined(WITH_THREAD)
#define USE_CTERMID_R
#endif

#if defined(HAVE_TMPNAM_R) && defined(WITH_THREAD)
#define USE_TMPNAM_R
#endif

/* choose the appropriate stat and fstat functions and return structs */
#define STRUCT_STAT struct stat

#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#else
#if defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_SYS_MKDEV_H)
#include <sys/mkdev.h>
#endif
#endif

extern void __seterrno(void); // this function set errno depend on IoErr() return value


//+ convertenviron()
/*
** Return a dictionary corresponding to the AmigaDOS environment dictionary.
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
    struct FileInfoBlock fib;
    char * buffer;
    PyObject *v;
    struct List *localVars;

    *dglobal    = PyDict_New();
    *dlocal     = PyDict_New();
    *dboth      = PyDict_New();
    *daliases   = PyDict_New();

    if (!(*dglobal && *dlocal && *dboth && *daliases))
    {
        if (*dglobal) {Py_DECREF(*dglobal);}
        if (*dlocal) {Py_DECREF(*dlocal);}
        if (*dboth) {Py_DECREF(*dboth);}
        if (*daliases) {Py_DECREF(*daliases);}

        return 0;
    }

    /* Read global vars from ENV:
    ** Put them in 'dglob' and in 'dboth'.
    */

    lock = Lock("ENV:", ACCESS_READ);
    if (lock)
    {
        if (Examine(lock, &fib))
        {
            while (ExNext(lock, &fib))
            {
                if (fib.fib_DirEntryType < 0)
                {
                    buffer = malloc(fib.fib_Size + 1);
                    if (buffer)
                    {
                        int len;

                        len = GetVar(fib.fib_FileName, buffer, fib.fib_Size + 1, GVF_GLOBAL_ONLY);
                        if (len >= 0)
                        {
                            // ignoring errors
                            v = PyString_FromString(buffer);
                            if (v)
                            {
                                PyDict_SetItemString(*dglobal, fib.fib_FileName, v);
                                PyDict_SetItemString(*dboth, fib.fib_FileName, v);
                                Py_DECREF(v);
                            }
                            else
                                PyErr_Clear();
                        }

                        free(buffer);
                    }
                }
            }
        }

        UnLock(lock);
    }

    /* Loop on local shell environment variables and aliases.
    ** Put variables in 'dlocal' and 'dboth'.
    ** Put aliases in 'daliases'.
    **
    ** Note: local shell variables are put in 'dboth' as globals. So, if
    ** there is a local variable with the same name that a global, the
    ** global is overwriten by the local, as it should be.
    */

    localVars = (struct List *) &((struct Process *)FindTask(NULL))->pr_LocalVars;
    if(!IsListEmpty(localVars))
    {
        struct LocalVar * var;

        ForeachNode(localVars, var)
        {
            buffer = malloc(var->lv_Len+1);
            if (buffer)
            {
                // copy variable name
                strncpy(buffer, var->lv_Value, var->lv_Len);
                buffer[var->lv_Len] = '\0';

                // put it, after a 'Pythonization', in right dictionaries
                v = PyString_FromString(buffer);
                if (v)
                {
                    if (var->lv_Node.ln_Type == LV_VAR)
                    {
                        PyDict_SetItemString(*dlocal, var->lv_Node.ln_Name, v);
                        PyDict_SetItemString(*dboth, var->lv_Node.ln_Name, v);
                    }
                    else if (var->lv_Node.ln_Type == LV_ALIAS)
                        PyDict_SetItemString(*daliases, var->lv_Node.ln_Name, v);

                    Py_DECREF(v);
                }

                free(buffer);
            }
        }
    }

    return TRUE;
}//-

/***********************************************************
** Set a Morphos-specific error from errno, and return NULL
*/

//+ morphos_error()
static PyObject *
morphos_error(void)
{
    if (IoErr() || !errno)
        __seterrno();
    return PyErr_SetFromErrno(PyExc_OSError);
}//-

//+ morphos_error_with_filename()
static PyObject *
morphos_error_with_filename(char* name)
{
    if (IoErr() || !errno)
        __seterrno();
    return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
}//-

//+ morphos_error_with_allocated_filename()
static PyObject *
morphos_error_with_allocated_filename(char* name)
{
    PyObject *rc;

    if (IoErr() || !errno)
        __seterrno();
    rc = PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
    PyMem_Free(name);
    return rc;
}//-

/**************************
** MORPHOS generic methods
*/

//+ morphos_fildes()
static PyObject *
morphos_fildes(PyObject *fdobj, int (*func)(int))
{
    int fd;
    int res;
    fd = PyObject_AsFileDescriptor(fdobj);
    if (fd < 0)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_1str()
static PyObject *
morphos_1str(PyObject *args, char *format, int (*func)(const char*))
{
    char *path1 = NULL;
    int res;

    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path1))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(path1);
    PyMem_Free(path1);
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_2str()
static PyObject *
morphos_2str(PyObject *args, char *format, int (*func)(const char *, const char *))
{
    char *path1 = NULL, *path2 = NULL;
    int res;

    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path1,
                          Py_FileSystemDefaultEncoding, &path2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1, path2);
    Py_END_ALLOW_THREADS
    PyMem_Free(path1);
    PyMem_Free(path2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ stat_result_fields[]
PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
Posix/windows: If your platform supports st_blksize, st_blocks, or st_rdev,\n\
they are available as attributes only.\n\
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
    {0}
};//-

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

//+ stat_result_desc
static PyStructSequence_Desc stat_result_desc = {
    "stat_result", /* name */
    stat_result__doc__, /* doc */
    stat_result_fields,
    10
};//-

//+ statvfs_result_fields[]
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
};//-

//+ statvfs_result_desc
static PyStructSequence_Desc statvfs_result_desc = {
    "statvfs_result", /* name */
    statvfs_result__doc__, /* doc */
    statvfs_result_fields,
    10
};//-

static PyTypeObject StatResultType;
static PyTypeObject StatVFSResultType;
static newfunc structseq_new;

//+ statresult_new()
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
}//-

/* If true, st_time is float. */
static int _stat_float_times = 0;

//+ stat_float_times()
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
}//-

//+ fill_time()
static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
    PyObject *fval,*ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
    ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
    ival = PyInt_FromLong((long)sec);
#endif
    if (_stat_float_times) {
        fval = PyFloat_FromDouble(sec + 1e-9*nsec);
    } else {
        fval = ival;
        Py_INCREF(fval);
    }
    PyStructSequence_SET_ITEM(v, index, ival);
    PyStructSequence_SET_ITEM(v, index+3, fval);
}//-

//+ _pystat_fromstructstat()
/*
** pack a system stat C structure into the Python stat tuple
** (used by morphos_stat() and morphos_fstat())
*/
static PyObject*
_pystat_fromstructstat(STRUCT_STAT st)
{
    unsigned long ansec, mnsec, cnsec;
    PyObject *v = PyStructSequence_New(&StatResultType);
    if (v == NULL)
        return NULL;

        PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long)st.st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyStructSequence_SET_ITEM(v, 1,
                  PyLong_FromLongLong((PY_LONG_LONG)st.st_ino));
#else
        PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long)st.st_ino));
#endif
#if defined(HAVE_LONG_LONG) && !defined(MS_WINDOWS)
        PyStructSequence_SET_ITEM(v, 2,
                  PyLong_FromLongLong((PY_LONG_LONG)st.st_dev));
#else
        PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long)st.st_dev));
#endif
        PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long)st.st_nlink));
        PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long)st.st_uid));
        PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long)st.st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyStructSequence_SET_ITEM(v, 6,
                  PyLong_FromLongLong((PY_LONG_LONG)st.st_size));
#else
        PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong(st.st_size));
#endif

#ifdef HAVE_STAT_TV_NSEC
    ansec = st.st_atim.tv_nsec;
    mnsec = st.st_mtim.tv_nsec;
    cnsec = st.st_ctim.tv_nsec;
#else
    ansec = mnsec = cnsec = 0;
#endif
    fill_time(v, 7, st.st_atime, ansec);
    fill_time(v, 8, st.st_mtime, mnsec);
    fill_time(v, 9, st.st_ctime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
             PyInt_FromLong((long)st.st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
             PyInt_FromLong((long)st.st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
             PyInt_FromLong((long)st.st_rdev));
#endif

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}//-

//+ morphos_do_stat()
static PyObject *
morphos_do_stat(PyObject *self, PyObject *args,
          char *format,
          int (*statfunc)(const char *, STRUCT_STAT *),
          char *wformat,
          int (*wstatfunc)(const Py_UNICODE *, STRUCT_STAT *))
{
    STRUCT_STAT st;
    char *path = NULL;  /* pass this to stat; do not free() it */
    char *pathfree = NULL;  /* this memory must be free'd */
    int res;


    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path))
        return NULL;
    pathfree = path;

    Py_BEGIN_ALLOW_THREADS
    res = (*statfunc)(path, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return morphos_error_with_allocated_filename(pathfree);

    PyMem_Free(pathfree);
    return _pystat_fromstructstat(st);
}//-

/******************
** Morphos methods
*/

//+ morphos_access()
PyDoc_STRVAR(morphos_access__doc__,
"access(path, mode) -> 1 if granted, 0 otherwise\n\n\
Use the real uid/gid to test for access to a path.  Note that most\n\
operations will use the effective uid/gid, therefore this routine can\n\
be used in a suid/sgid environment to test if the invoking user has the\n\
specified access to the path.  The mode argument can be F_OK to test\n\
existence, or the inclusive-OR of R_OK, W_OK, and X_OK.");

static PyObject *
morphos_access(PyObject *self, PyObject *args)
{
    char *path;
    int mode;
    int res;

    if (!PyArg_ParseTuple(args, "si:access", &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = access(path, mode);
    Py_END_ALLOW_THREADS
    return(PyBool_FromLong(res == 0));
}//-

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
//+ morphos_ctermid()
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
        return(morphos_error());
    return(PyString_FromString(buffer));
}//-
#endif

PyDoc_STRVAR(morphos_chdir__doc__,
"chdir(path)\n\n\
Change the current working directory to the specified path.");

static PyObject *
morphos_chdir(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "et:chdir", chdir);
}

#ifdef HAVE_FCHDIR
PyDoc_STRVAR(morphos_fchdir__doc__,
"fchdir(fildes)\n\n\
Change to the directory of the given file descriptor.  fildes must be\n\
opened on a directory, not a file.");

static PyObject *
morphos_fchdir(PyObject *self, PyObject *fdobj)
{
    return morphos_fildes(fdobj, fchdir);
}
#endif /* HAVE_FCHDIR */

PyDoc_STRVAR(morphos_chmod__doc__,
"chmod(path, mode)\n\n\
Change the access permissions of a file.");

static PyObject *
morphos_chmod(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int i;
    int res;
    if (!PyArg_ParseTuple(args, "eti:chmod", Py_FileSystemDefaultEncoding,
                          &path, &i))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = chmod(path, i);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef HAVE_CHROOT
PyDoc_STRVAR(morphos_chroot__doc__,
"chroot(path)\n\n\
Change root directory to path.");

static PyObject *
morphos_chroot(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "et:chroot", chroot);
}
#endif

#ifdef HAVE_FSYNC
PyDoc_STRVAR(morphos_fsync__doc__,
"fsync(fildes)\n\n\
force write of file with filedescriptor to disk.");

static PyObject *
morphos_fsync(PyObject *self, PyObject *fdobj)
{
    return morphos_fildes(fdobj, fsync);
}
#endif /* HAVE_FSYNC */

#ifdef HAVE_FDATASYNC
PyDoc_STRVAR(morphos_fdatasync__doc__,
"fdatasync(fildes)\n\n\
force write of file with filedescriptor to disk.\n\
 does not force update of metadata.");

static PyObject *
morphos_fdatasync(PyObject *self, PyObject *fdobj)
{
       return morphos_fildes(fdobj, fdatasync);
}/
#endif /* HAVE_FDATASYNC */

#ifdef HAVE_CHOWN
//+ morphos_chown()
PyDoc_STRVAR(morphos_chown__doc__,
"chown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.");

static PyObject *
morphos_chown(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "etii:chown",
                          Py_FileSystemDefaultEncoding, &path,
                          &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = chown(path, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif /* HAVE_CHOWN */

#ifdef HAVE_LCHOWN
//+ morphos_lchown()
PyDoc_STRVAR(morphos_lchown__doc__,
"lchown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
This function will not follow symbolic links.");

static PyObject *
morphos_lchown(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "etii:lchown",
                          Py_FileSystemDefaultEncoding, &path,
                          &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = lchown(path, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif /* HAVE_LCHOWN */

//+ morphos_getcwd()
PyDoc_STRVAR(morphos_getcwd__doc__,
"getcwd() -> path\n\n\
Return a string representing the current working directory.");

static PyObject *
morphos_getcwd(PyObject *self, PyObject *noargs)
{
    char buf[1026];
    char *res;

    Py_BEGIN_ALLOW_THREADS
    res = getcwd(buf, sizeof buf);
    Py_END_ALLOW_THREADS
    if (res == NULL)
        return morphos_error();
    return PyString_FromString(buf);
}//-

#ifdef Py_USING_UNICODE
//+ morphos_getcwdu()
PyDoc_STRVAR(morphos_getcwdu__doc__,
"getcwdu() -> path\n\n\
Return a unicode string representing the current working directory.");

static PyObject *
morphos_getcwdu(PyObject *self, PyObject *noargs)
{
    char buf[1026];
    char *res;

    Py_BEGIN_ALLOW_THREADS
    res = getcwd(buf, sizeof buf);
    Py_END_ALLOW_THREADS
    if (res == NULL)
        return morphos_error();
    return PyUnicode_Decode(buf, strlen(buf), Py_FileSystemDefaultEncoding,"strict");
}//-
#endif

//+ morphos_listdir()
PyDoc_STRVAR(morphos_listdir__doc__,
"listdir(path) -> list_of_strings\n\n\
Return a list containing the names of the entries in the directory.\n\
\n\
    path: path of directory to list\n\
\n\
The list is in arbitrary order.  It does not include the special\n\
entries '.' and '..' even if they are present in the directory.");

static PyObject *
morphos_listdir(PyObject *self, PyObject *args)
{
    /* XXX Should redo this putting the (now four) versions of opendir
       in separate files instead of having them all here... */
    char *name = NULL;
    PyObject *d = NULL, *v;
    int arg_is_unicode = 1;

    if (!PyArg_ParseTuple(args, "U:listdir", &v))
    {
        arg_is_unicode = 0;
        PyErr_Clear();
    }

    if (PyArg_ParseTuple(args, "et:listdir", Py_FileSystemDefaultEncoding, &name))
    {
        BPTR lock;

        lock = Lock(name, SHARED_LOCK);
        if (lock != NULL)
        {
            struct FileInfoBlock fib;

            if (Examine(lock, &fib))
            {
                d = PyList_New(0);
                if (d != NULL)
                {
                    while (ExNext(lock, &fib) != 0)
                    {
                        v = PyString_FromStringAndSize(fib.fib_FileName, strlen(fib.fib_FileName));
                        if (v == NULL)
                        {
                            Py_DECREF(d);
                            d = NULL;
                            break;
                        }
#ifdef Py_USING_UNICODE
                        if (arg_is_unicode)
                        {
                            PyObject *w;

                            w = PyUnicode_FromEncodedObject(v, Py_FileSystemDefaultEncoding, "strict");
                            if (w != NULL)
                            {
                                Py_DECREF(v);
                                v = w;
                            }
                            else
                            {
                                /* fall back to the original byte string, as
                                   discussed in patch #683592 */
                                PyErr_Clear();
                            }
                        }
#endif
                        if (PyList_Append(d, v) != 0)
                        {
                            Py_DECREF(v);
                            Py_DECREF(d);
                            d = NULL;
                            break;
                        }

                        Py_DECREF(v);
                    }

                    // Check about error
                    if (IoErr() != ERROR_NO_MORE_ENTRIES)
                    {
                        morphos_error_with_allocated_filename(name);
                        Py_DECREF(d);
                        d = NULL;
                    }
                }
            }

            UnLock(lock);
        }
        else
            morphos_error_with_allocated_filename(name);
    }

    return d;
}//-  /* end of morphos_listdir */

//+ morphos_mkdir()
PyDoc_STRVAR(morphos_mkdir__doc__,
"mkdir(path [, mode=0777])\n\n\
Create a directory.");

static PyObject *
morphos_mkdir(PyObject *self, PyObject *args)
{
    int res;
    char *path = NULL;
    int mode = 0777;

    if (!PyArg_ParseTuple(args, "et|i:mkdir",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = mkdir(path, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_nice()
PyDoc_STRVAR(morphos_nice__doc__,
"nice(inc) -> new_priority\n\n\
Decrease the priority of process by inc and return the new priority.");

static PyObject *
morphos_nice(PyObject *self, PyObject *args)
{
    int increment, value;

    if (!PyArg_ParseTuple(args, "i:nice", &increment))
        return NULL;

    value = SetTaskPri(FindTask(NULL), increment);
    return PyInt_FromLong((long) value);
}//-

//+ morphos_rename()
PyDoc_STRVAR(morphos_rename__doc__,
"rename(old, new)\n\n\
Rename a file or directory.");

static PyObject *
morphos_rename(PyObject *self, PyObject *args)
{
    char *path1 = NULL, *path2 = NULL;
    int res;

    if (!PyArg_ParseTuple(args, "etet:rename",
                          Py_FileSystemDefaultEncoding, &path1,
                          Py_FileSystemDefaultEncoding, &path2))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = Rename(path1, path2);
    Py_END_ALLOW_THREADS

    PyMem_Free(path1);
    PyMem_Free(path2);

    if (!res)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();

    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_rmdir()
PyDoc_STRVAR(morphos_rmdir__doc__,
"rmdir(path)\n\n\
Remove a directory.");

static PyObject *
morphos_rmdir(PyObject *self, PyObject *args)
{
    char *dir = NULL;
    int res;

    if (!PyArg_ParseTuple(args, "et:rmdir", Py_FileSystemDefaultEncoding, &dir))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = DeleteFile(dir);
    Py_END_ALLOW_THREADS
    if (res == FALSE)
        return morphos_error_with_allocated_filename(dir);
    PyMem_Free(dir);
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_stat()
PyDoc_STRVAR(morphos_stat__doc__,
"stat(path) -> stat result\n\n\
Perform a stat system call on the given path.");

static PyObject *
morphos_stat(PyObject *self, PyObject *args)
{
    return morphos_do_stat(self, args, "et:stat", stat, NULL, NULL);
}//-

//+ morphos_system()
PyDoc_STRVAR(morphos_system__doc__,
"system(command) -> exit_status\n\n\
Execute the command (a string) in a subshell.");

static PyObject *
morphos_system(PyObject *self, PyObject *args)
{
    char *command;
    long sts;
    if (!PyArg_ParseTuple(args, "s:system", &command))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    sts = SystemTagList(command, NULL);
    Py_END_ALLOW_THREADS

    return PyInt_FromLong(sts);
}//-

//+ morphos_umask()
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
    return PyInt_FromLong((long)i);
}//-

//+ morphos_unlink()
PyDoc_STRVAR(morphos_unlink__doc__,
"unlink(path)\n\n\
Remove a file (same as remove(path)).");

PyDoc_STRVAR(morphos_remove__doc__,
"remove(path)\n\n\
Remove a file (same as unlink(path)).");

static PyObject *
morphos_unlink(PyObject *self, PyObject *args)
{
    return morphos_1str(args, "et:remove", unlink);
}//-

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
}//-
#endif /* HAVE_UNAME */

//+ extract_time()
static int
extract_time(PyObject *t, long* sec, long* usec)
{
    long intval;
    if (PyFloat_Check(t)) {
        double tval = PyFloat_AsDouble(t);
        PyObject *intobj = t->ob_type->tp_as_number->nb_int(t);
        if (!intobj)
            return -1;
        intval = PyInt_AsLong(intobj);
        Py_DECREF(intobj);
        *sec = intval;
        *usec = (long)((tval - intval) * 1e6); /* can't exceed 1000000 */
        if (*usec < 0)
            /* If rounding gave us a negative number,
               truncate.  */
            *usec = 0;
        return 0;
    }
    intval = PyInt_AsLong(t);
    if (intval == -1 && PyErr_Occurred())
        return -1;
    *sec = intval;
    *usec = 0;
        return 0;
}//-

//+ morphos_utime()
PyDoc_STRVAR(morphos_utime__doc__,
"utime(path, (atime, utime))\n\
utime(path, None)\n\n\
Set the access and modified time of the file to the given values.  If the\n\
second form is used, set the access and modified times to the current time.");

static PyObject *
morphos_utime(PyObject *self, PyObject *args)
{
    char *path;
    long atime, mtime, ausec, musec;
    int res;
    PyObject* arg;
    struct utimbuf buf;
    int have_unicode_filename = 0;

    if (!have_unicode_filename && \
        !PyArg_ParseTuple(args, "sO:utime", &path, &arg))
        return NULL;
    if (arg == Py_None) {
        /* optional time values not given */
        Py_BEGIN_ALLOW_THREADS
        res = utime(path, NULL);
        Py_END_ALLOW_THREADS
    }
    else if (!PyTuple_Check(arg) || PyTuple_Size(arg) != 2) {
        PyErr_SetString(PyExc_TypeError,
                "utime() arg 2 must be a tuple (atime, mtime)");
        return NULL;
    }
    else {
        if (extract_time(PyTuple_GET_ITEM(arg, 0),
                 &atime, &ausec) == -1)
            return NULL;
        if (extract_time(PyTuple_GET_ITEM(arg, 1),
                 &mtime, &musec) == -1)
            return NULL;
        buf.actime = atime;
        buf.modtime   = mtime;
        Py_BEGIN_ALLOW_THREADS
        res = utime(path, &buf);
        Py_END_ALLOW_THREADS
    }
    if (res < 0)
        return morphos_error_with_filename(path);
    Py_INCREF(Py_None);
    return Py_None;
}//-

/*********************
** Process operations
*/

//+ morphos__exit()
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
    return NULL; /* Make gcc -Wall happy */
}//-

#if defined(HAVE_EXECV) || defined(HAVE_SPAWNV)
//+ free_string_array()
static void
free_string_array(char **array, int count)
{
    int i;
    for (i = 0; i < count; i++)
        PyMem_Free(array[i]);
    PyMem_DEL(array);
}//-
#endif

//+ morphos_getegid()
PyDoc_STRVAR(morphos_getegid__doc__,
"getegid() -> egid\n\n\
Return the current process's effective group id.");

static PyObject *
morphos_getegid(PyObject *self, PyObject *noargs)
{
    return PyInt_FromLong((long)getegid());
}//-

//+ morphos_geteuid()
PyDoc_STRVAR(morphos_geteuid__doc__,
"geteuid() -> euid\n\n\
Return the current process's effective user id.");

static PyObject *
morphos_geteuid(PyObject *self, PyObject *noargs)
{
    return PyInt_FromLong((long)geteuid());
}//-

//+ morphos_getgid()
PyDoc_STRVAR(morphos_getgid__doc__,
"getgid() -> gid\n\n\
Return the current process's group id.");

static PyObject *
morphos_getgid(PyObject *self, PyObject *noargs)
{
    return PyInt_FromLong((long)getgid());
}//-

//+ morphos_getpid()
PyDoc_STRVAR(morphos_getpid__doc__,
"getpid() -> pid\n\n\
Return the current process id");

static PyObject *
morphos_getpid(PyObject *self, PyObject *noargs)
{
    long pid;

    if (!NewGetTaskAttrsA(NULL, &pid, sizeof(pid), TASKINFOTYPE_PID, 0))
        pid = (long)FindTask(NULL);

    return PyInt_FromLong(pid);
}//-

//+ morphos_getgroups()
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
                PyObject *o = PyInt_FromLong((long)grouplist[i]);
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
}//-

//+ morphos_getpgrp()
PyDoc_STRVAR(morphos_getpgrp__doc__,
"getpgrp() -> pgrp\n\n\
Return the current process group id.");

static PyObject *
morphos_getpgrp(PyObject *self, PyObject *noargs)
{
#ifdef GETPGRP_HAVE_ARG
    return PyInt_FromLong((long)getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
    return PyInt_FromLong((long)getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}//-

//+ morphos_getlogin()
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
        result = PyString_FromString(name);
    errno = old_errno;

    return result;
}//-

//+ morphos_getuid()
PyDoc_STRVAR(morphos_getuid__doc__,
"getuid() -> uid\n\n\
Return the current process's user id.");

static PyObject *
morphos_getuid(PyObject *self, PyObject *noargs)
{
    return PyInt_FromLong((long)getuid());
}//-

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
}//-
#endif

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
}//-

//+ morphos_setuid()
PyDoc_STRVAR(morphos_setuid__doc__,
"setuid(uid)\n\n\
Set the current process's user id.");

static PyObject *
morphos_setuid(PyObject *self, PyObject *args)
{
    int uid;
    if (!PyArg_ParseTuple(args, "i:setuid", &uid))
        return NULL;
    if (setuid(uid) < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_setreuid()
PyDoc_STRVAR(morphos_setreuid__doc__,
"seteuid(ruid, euid)\n\n\
Set the current process's real and effective user ids.");

static PyObject *
morphos_setreuid (PyObject *self, PyObject *args)
{
    int ruid, euid;
    if (!PyArg_ParseTuple(args, "ii", &ruid, &euid)) {
        return NULL;
    } else if (setreuid(ruid, euid) < 0) {
        return morphos_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}//-

//+ morphos_setregid()
PyDoc_STRVAR(morphos_setregid__doc__,
"setegid(rgid, egid)\n\n\
Set the current process's real and effective group ids.");

static PyObject *
morphos_setregid (PyObject *self, PyObject *args)
{
    int rgid, egid;
    if (!PyArg_ParseTuple(args, "ii", &rgid, &egid)) {
        return NULL;
    } else if (setregid(rgid, egid) < 0) {
        return morphos_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}//-

//+ morphos_setgid()
PyDoc_STRVAR(morphos_setgid__doc__,
"setgid(gid)\n\n\
Set the current process's group id.");

static PyObject *
morphos_setgid(PyObject *self, PyObject *args)
{
    int gid;
    if (!PyArg_ParseTuple(args, "i:setgid", &gid))
        return NULL;
    if (setgid(gid) < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_setgroups()
PyDoc_STRVAR(morphos_setgroups__doc__,
"setgroups(list)\n\n\
Set the groups of the current process to list.");

static PyObject *
morphos_setgroups(PyObject *self, PyObject *args)
{
    PyObject *groups;
    int i, len;
    gid_t grouplist[MAX_GROUPS];

    if (!PyArg_ParseTuple(args, "O:setgid", &groups))
        return NULL;
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
        if (!PyInt_Check(elem)) {
            PyErr_SetString(PyExc_TypeError,
                    "groups must be integers");
            Py_DECREF(elem);
            return NULL;
        }
        /* XXX: check that value fits into gid_t. */
        grouplist[i] = PyInt_AsLong(elem);
        Py_DECREF(elem);
    }

    if (setgroups(len, grouplist) < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

//+ morphos_lstat()
PyDoc_STRVAR(morphos_lstat__doc__,
"lstat(path) -> stat result\n\n\
Like stat(path), but do not follow symbolic links.");

static PyObject *
morphos_lstat(PyObject *self, PyObject *args)
{
    return morphos_do_stat(self, args, "et:lstat", lstat, NULL, NULL);
}//-

#ifdef HAVE_READLINK
//+ morphos_readlink()
PyDoc_STRVAR(morphos_readlink__doc__,
"readlink(path) -> path\n\n\
Return a string representing the path to which the symbolic link points.");

static PyObject *
morphos_readlink(PyObject *self, PyObject *args)
{
    char buf[MAXPATHLEN];
    char *path;
    int n;
    if (!PyArg_ParseTuple(args, "s:readlink", &path))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    n = readlink(path, buf, (int) sizeof buf);
    Py_END_ALLOW_THREADS
    if (n < 0)
        return morphos_error_with_filename(path);
    return PyString_FromStringAndSize(buf, n);
}//-
#endif /* HAVE_READLINK */

#if 0

// XXX: hard link on morphos ?

//+ morphos_link()
PyDoc_STRVAR(morphos_link__doc__,
"link(src, dst)\n\n\
Create a hard link to a file.");

static PyObject *
morphos_link(PyObject *self, PyObject *args)
{
    char *path1 = NULL, *path2 = NULL;
    int res = -1;
    BPTR lock;

    if (!PyArg_ParseTuple(args, "etet:link",
                          Py_FileSystemDefaultEncoding, &path1,
                          Py_FileSystemDefaultEncoding, &path2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    lock = Lock(path1, SHARED_LOCK);
    if (lock)
    {
        res = !MakeLink(path2, lock, FALSE);
        if (res)
            __seterrno();
        UnLock(lock);
    }
    else
        __seterrno();
    Py_END_ALLOW_THREADS
    PyMem_Free(path1);
    PyMem_Free(path2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif

//+ morphos_symlink()
PyDoc_STRVAR(morphos_symlink__doc__,
"symlink(src, dst)\n\n\
Create a symbolic link pointing to src named dst.");

static PyObject *
morphos_symlink(PyObject *self, PyObject *args)
{
    char *path1 = NULL, *path2 = NULL;
    int res = -1;
    BPTR lock;

    if (!PyArg_ParseTuple(args, "etet:symlink",
                          Py_FileSystemDefaultEncoding, &path1,
                          Py_FileSystemDefaultEncoding, &path2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS

    /* XXX don't check for empty path1, because empty src is equivalent to 'current directory'.
    ** or we can link the current directory
    */
    lock = Lock(path1, SHARED_LOCK);
    if (lock)
    {
        res = !MakeLink(path2, (LONG)path1, TRUE);
        if (res)
            __seterrno();
        UnLock(lock);
    }
    else
        __seterrno();
    Py_END_ALLOW_THREADS
    PyMem_Free(path1);
    PyMem_Free(path2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;

}//-

#ifdef HAVE_TIMES
#ifndef HZ
#define HZ 60 /* Universal constant :-) */
#endif /* HZ */

//+ morphos_times()
PyDoc_STRVAR(morphos_times__doc__,
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\n\
Return a tuple of floating point numbers indicating process times.");

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
                 (double)t.tms_utime / HZ,
                 (double)t.tms_stime / HZ,
                 (double)t.tms_cutime / HZ,
                 (double)t.tms_cstime / HZ,
                 (double)c / HZ);
}//-
#endif /* HAVE_TIMES */

//+ morphos_setsid()
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
}//-

/***************************************
** Functions acting on file descriptors
*/

//+ morphos_GetOSFileHandle
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

    Py_BEGIN_ALLOW_THREADS
    fh = __get_handle(fd);
    Py_END_ALLOW_THREADS
    if (NULL == fh)
        return morphos_error();
    return PyInt_FromLong((long)fh);
}
//-

//+ morphos_open()
PyDoc_STRVAR(morphos_open__doc__,
"open(filename, flag [, mode=0777]) -> fd\n\n\
Open a file (for low level IO).");

static PyObject *
morphos_open(PyObject *self, PyObject *args)
{
    char *file = NULL;
    int flag;
    int mode = 0777;
    int fd;

    if (!PyArg_ParseTuple(args, "eti|i",
                          Py_FileSystemDefaultEncoding, &file,
                          &flag, &mode))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    fd = open(file, flag, mode);
    Py_END_ALLOW_THREADS
    if (fd < 0)
        return morphos_error_with_allocated_filename(file);
    PyMem_Free(file);
    return PyInt_FromLong((long)fd);
}//-

//+ morphos_close()
PyDoc_STRVAR(morphos_close__doc__,
"close(fd)\n\n\
Close a file descriptor (for low level IO).");

static PyObject *
morphos_close(PyObject *self, PyObject *args)
{
    int fd, res;
    if (!PyArg_ParseTuple(args, "i:close", &fd))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = close(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-

#ifdef PORTED
//+ morphos_dup()
PyDoc_STRVAR(morphos_dup__doc__,
"dup(fd) -> fd2\n\n\
Return a duplicate of a file descriptor.");

static PyObject *
morphos_dup(PyObject *self, PyObject *args)
{
    int fd;
    if (!PyArg_ParseTuple(args, "i:dup", &fd))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    fd = dup(fd);
    Py_END_ALLOW_THREADS
    if (fd < 0)
        return morphos_error();
    return PyInt_FromLong((long)fd);
}//-

//+ morphos_dup2()
PyDoc_STRVAR(morphos_dup2__doc__,
"dup2(fd, fd2)\n\n\
Duplicate file descriptor.");

static PyObject *
morphos_dup2(PyObject *self, PyObject *args)
{
    int fd, fd2, res;
    if (!PyArg_ParseTuple(args, "ii:dup2", &fd, &fd2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = dup2(fd, fd2);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif /* PORTED */

//+ morphos_lseek()
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

#if !defined(HAVE_LARGEFILE_SUPPORT)
    pos = PyInt_AsLong(posobj);
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
    return PyInt_FromLong(res);
#else
    return PyLong_FromLongLong(res);
#endif
}//-

//+ morphos_read()
PyDoc_STRVAR(morphos_read__doc__,
"read(fd, buffersize) -> string\n\n\
Read a file descriptor.");

static PyObject *
morphos_read(PyObject *self, PyObject *args)
{
    int fd, size, n;
    PyObject *buffer;
    if (!PyArg_ParseTuple(args, "ii:read", &fd, &size))
        return NULL;
    buffer = PyString_FromStringAndSize((char *)NULL, size);
    if (buffer == NULL)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    n = read(fd, PyString_AsString(buffer), size);
    Py_END_ALLOW_THREADS
    if (n < 0) {
        Py_DECREF(buffer);
        return morphos_error();
    }
    if (n != size)
        _PyString_Resize(&buffer, n);
    return buffer;
}//-

//+ morphos_write()
PyDoc_STRVAR(morphos_write__doc__,
"write(fd, string) -> byteswritten\n\n\
Write a string to a file descriptor.");

static PyObject *
morphos_write(PyObject *self, PyObject *args)
{
    int fd, size;
    char *buffer;
    if (!PyArg_ParseTuple(args, "is#:write", &fd, &buffer, &size))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    size = write(fd, buffer, size);
    Py_END_ALLOW_THREADS
    if (size < 0)
        return morphos_error();
    return PyInt_FromLong((long)size);
}//-

//+ morphos_fstat()
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
    Py_BEGIN_ALLOW_THREADS
    res = fstat(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return morphos_error();

    return _pystat_fromstructstat(st);
}//-

//+ morphos_fdopen()
PyDoc_STRVAR(morphos_fdopen__doc__,
"fdopen(fd [, mode='r' [, bufsize]]) -> file_object\n\n\
Return an open file object connected to a file descriptor.");

static PyObject *
morphos_fdopen(PyObject *self, PyObject *args)
{
    int fd;
    char *mode = "r";
    int bufsize = -1;
    FILE *fp;
    PyObject *f;
    if (!PyArg_ParseTuple(args, "i|si", &fd, &mode, &bufsize))
        return NULL;

    if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') {
        PyErr_Format(PyExc_ValueError,
                 "invalid file mode '%s'", mode);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    fp = fdopen(fd, mode);
    Py_END_ALLOW_THREADS
    if (fp == NULL)
        return morphos_error();
    f = PyFile_FromFile(fp, "<fdopen>", mode, fclose);
    if (f != NULL)
        PyFile_SetBufSize(f, bufsize);
    return f;
}//-

//+ morphos_isatty()
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
    return PyBool_FromLong(isatty(fd));
}//-

#ifdef HAVE_MKFIFO
//+ morphos_mkfifo()
PyDoc_STRVAR(morphos_mkfifo__doc__,
"mkfifo(filename [, mode=0666])\n\n\
Create a FIFO (a POSIX named pipe).");

static PyObject *
morphos_mkfifo(PyObject *self, PyObject *args)
{
    char *filename;
    int mode = 0666;
    int res;
    if (!PyArg_ParseTuple(args, "s|i:mkfifo", &filename, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = mkfifo(filename, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return morphos_error();
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif

#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
//+ morphos_mknod()
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
}//-
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
    return PyInt_FromLong((long)major(device));
}//-

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
    return PyInt_FromLong((long)minor(device));
}//-

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
    return PyInt_FromLong((long)makedev(major, minor));
}//-
#endif /* device macros */

#ifdef HAVE_FTRUNCATE
//+ morphos_ftruncate()
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

#if !defined(HAVE_LARGEFILE_SUPPORT)
    length = PyInt_AsLong(lenobj);
#else
    length = PyLong_Check(lenobj) ?
        PyLong_AsLongLong(lenobj) : PyInt_AsLong(lenobj);
#endif
    if (PyErr_Occurred())
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = ftruncate(fd, length);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}//-
#endif

#ifdef HAVE_PUTENV
//+ morphos_putenv()
PyDoc_STRVAR(morphos_putenv__doc__,
"putenv(key, value)\n\n\
Change or add an environment variable.");

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *morphos_putenv_garbage;

static PyObject *
morphos_putenv(PyObject *self, PyObject *args)
{
    char *s1, *s2;
    char *new;
    PyObject *newstr;
    size_t len;

    if (!PyArg_ParseTuple(args, "ss:putenv", &s1, &s2))
        return NULL;

    /* XXX This can leak memory -- not easy to fix :-( */
    len = strlen(s1) + strlen(s2) + 2;
    /* len includes space for a trailing \0; the size arg to
       PyString_FromStringAndSize does not count that */
    newstr = PyString_FromStringAndSize(NULL, (int)len - 1);
    if (newstr == NULL)
        return PyErr_NoMemory();
    new = PyString_AS_STRING(newstr);
    PyOS_snprintf(new, len, "%s=%s", s1, s2);
    if (putenv(new)) {
                Py_DECREF(newstr);
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

    Py_INCREF(Py_None);
        return Py_None;
}//-
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
}//-
#endif /* unsetenv */

#ifdef HAVE_STRERROR
//+ morphos_strerror()
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
    return PyString_FromString(message);
}//-
#endif /* strerror */

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
}//-

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
}//-
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
}//-
#endif /* HAVE_STATVFS */

//+ morphos_tmpfile()
PyDoc_STRVAR(morphos_tmpfile__doc__,
"tmpfile() -> file object\n\n\
Create a temporary file with no directory entries.");

static PyObject *
morphos_tmpfile(PyObject *self, PyObject *noargs)
{
    pPyMorphOS_ThreadData_t td = GET_THREAD_DATA_PTR();
    struct Library * TimerBase = GET_THREAD_DATA(td, TimerBase);
    FILE *fp;
    char buf[32];
    struct EClockVal ecval;

    ReadEClock(&ecval);
    sprintf(buf, "t:tempfile_%p_%08x", FindTask(NULL), (unsigned int)ecval.ev_lo);

    fp = fopen(buf, "w+b");
    if (fp == NULL)
        return morphos_error();

    return PyFile_FromFile(fp, "<tmpfile>", "w+b", fclose);
}//-

//+ morphos_tmpnam()
PyDoc_STRVAR(morphos_tmpnam__doc__,
"tmpnam() -> string\n\n\
Return a unique name for a temporary file.");

static PyObject *
morphos_tmpnam(PyObject *self, PyObject *noargs)
{
    char buffer[L_tmpnam];
    char *name;

    if (PyErr_Warn(PyExc_RuntimeWarning,
          "tmpnam is a potential security risk to your program") < 0)
        return NULL;

#ifdef USE_TMPNAM_R
    name = tmpnam_r(buffer);
#else
    name = tmpnam(buffer);
#endif
    if (name == NULL) {
        PyErr_SetObject(PyExc_OSError,
                        Py_BuildValue("is", 0,
#ifdef USE_TMPNAM_R
                                      "unexpected NULL from tmpnam_r"
#else
                                      "unexpected NULL from tmpnam"
#endif
                                      ));
        return NULL;
    }
    return PyString_FromString(buffer);
}//-

//+ morphos_abort()
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
}//-

#ifdef HAVE_GETLOADAVG
//+ morphos_getloadavg()
PyDoc_STRVAR(morphos_getloadavg__doc__,
"getloadavg() -> (float, float, float)\n\n\
Return the number of processes in the system run queue averaged over\n\
the last 1, 5, and 15 minutes or raises OSError if the load average\n\
was unobtainable");

static PyObject *
morphos_getloadavg(PyObject *self, PyObject *args)
{
    double loadavg[3];
    if (getloadavg(loadavg, 3)!=3) {
        PyErr_SetString(PyExc_OSError, "Load averages are unobtainable");
        return NULL;
    } else
        return Py_BuildValue("ddd", loadavg[0], loadavg[1], loadavg[2]);
}//-
#endif

//+ morphos_urandom()
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
    if (! PyArg_ParseTuple(args, "i:urandom", &count))
        return NULL;
    if (count < 0)
        return PyErr_Format(PyExc_ValueError,
                    "negative argument not allowed");

    /* Allocate bytes */
    result = PyString_FromStringAndSize(NULL, count);
    if (result != NULL)
    {
        /* Get random data */

        s = PyString_AS_STRING(result);
        for (i=count >> 2; i; i--)
        {
            ULONG *p = (ULONG *)s;

            seed = FastRand(seed);
            *(p++) = seed;
        }

        for (; count % 4; count--)
        {
            seed = FastRand(seed);
            *(s++) = seed;
        }

        *s = '\0';
    }

    return result;
}//-

//+ morphos_AddPart()
PyDoc_STRVAR(morphos_AddPart__doc__,
"AddPart(string, string) -> (string)\n\n\
Join two path in one, result is the concatened path");

static PyObject *
morphos_AddPart(PyObject *self, PyObject *args)
{
    PyObject * obj = NULL;
    STRPTR dir1, dir2;
    STRPTR buffer;

    if (PyArg_ParseTuple(args, "etet:AddPart",
        Py_FileSystemDefaultEncoding, &dir1,
        Py_FileSystemDefaultEncoding, &dir2))
    {
        // if dir2 is an empty string, return dir1
        if (strlen(dir2) > 0)
        {
            int len = strlen(dir1) + strlen(dir2) + 2; // +2 = a possible '/' and '\0'

            obj = PyString_FromStringAndSize(NULL, len);
            if (obj)
            {
                BOOL res;

                buffer = PyString_AS_STRING(obj);
                strcpy(buffer, dir1);
                res = AddPart(buffer, dir2, len);
                if (res)
                    _PyString_Resize(&obj, strlen(buffer));
                else
                {
                    PyErr_SetString(PyExc_RuntimeError, "overflow during AddPart()");
                    obj = NULL;
                }
            }
            else
                PyErr_NoMemory();
        }
        else
        {
            obj = PyString_FromString(dir1);
            if (!obj)
                PyErr_NoMemory();
        }
    }

    Py_XINCREF(obj);
    return obj;
}//-

//+ morphos_uniqueid()
static PyObject *
morphos_uniqueid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromUnsignedLong(GetUniqueID());
}//-

//+ morphos_methods[]
static PyMethodDef morphos_methods[] = {
    {"access",  morphos_access, METH_VARARGS, morphos_access__doc__},
    {"chdir",   morphos_chdir, METH_VARARGS, morphos_chdir__doc__},
    {"chmod",   morphos_chmod, METH_VARARGS, morphos_chmod__doc__},
#ifdef HAVE_CHOWN
    {"chown",   morphos_chown, METH_VARARGS, morphos_chown__doc__},
#endif /* HAVE_CHOWN */
#ifdef HAVE_LCHOWN
    {"lchown",  morphos_lchown, METH_VARARGS, morphos_lchown__doc__},
#endif /* HAVE_LCHOWN */
#ifdef HAVE_CHROOT
    {"chroot",  morphos_chroot, METH_VARARGS, morphos_chroot__doc__},
#endif
#ifdef HAVE_CTERMID
    {"ctermid", morphos_ctermid, METH_NOARGS, morphos_ctermid__doc__},
#endif
    {"getcwd",  morphos_getcwd, METH_NOARGS, morphos_getcwd__doc__},
#ifdef Py_USING_UNICODE
    {"getcwdu", morphos_getcwdu, METH_NOARGS, morphos_getcwdu__doc__},
#endif
#if 0
    {"link",    morphos_link, METH_VARARGS, morphos_link__doc__},
#endif
    {"listdir", morphos_listdir, METH_VARARGS, morphos_listdir__doc__},
    {"lstat",   morphos_lstat, METH_VARARGS, morphos_lstat__doc__},
    {"mkdir",   morphos_mkdir, METH_VARARGS, morphos_mkdir__doc__},
    {"nice",    morphos_nice, METH_VARARGS, morphos_nice__doc__},
#ifdef HAVE_READLINK
    {"readlink",    morphos_readlink, METH_VARARGS, morphos_readlink__doc__},
#endif /* HAVE_READLINK */
    {"rename",  morphos_rename, METH_VARARGS, morphos_rename__doc__},
    {"rmdir",   morphos_rmdir, METH_VARARGS, morphos_rmdir__doc__},
    {"stat",    morphos_stat, METH_VARARGS, morphos_stat__doc__},
    {"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
    {"symlink", morphos_symlink, METH_VARARGS, morphos_symlink__doc__},
    {"system",  morphos_system, METH_VARARGS, morphos_system__doc__},
    {"umask",   morphos_umask, METH_VARARGS, morphos_umask__doc__},
#ifdef HAVE_UNAME
    {"uname",   morphos_uname, METH_NOARGS, morphos_uname__doc__},
#endif /* HAVE_UNAME */
    {"unlink",  morphos_unlink, METH_VARARGS, morphos_unlink__doc__},
    {"remove",  morphos_unlink, METH_VARARGS, morphos_remove__doc__},
    {"utime",   morphos_utime, METH_VARARGS, morphos_utime__doc__},
    {"_exit",   morphos__exit, METH_VARARGS, morphos__exit__doc__},
#ifdef HAVE_EXECV
    {"execv",   morphos_execv, METH_VARARGS, morphos_execv__doc__},
    {"execve",  morphos_execve, METH_VARARGS, morphos_execve__doc__},
#endif /* HAVE_EXECV */
    {"getegid", morphos_getegid, METH_NOARGS, morphos_getegid__doc__},
    {"geteuid", morphos_geteuid, METH_NOARGS, morphos_geteuid__doc__},
    {"getgid",  morphos_getgid, METH_NOARGS, morphos_getgid__doc__},
    {"getgroups",   morphos_getgroups, METH_NOARGS, morphos_getgroups__doc__},
    {"getpid",  morphos_getpid, METH_NOARGS, morphos_getpid__doc__},
    {"getpgrp", morphos_getpgrp, METH_NOARGS, morphos_getpgrp__doc__},
    {"getuid",  morphos_getuid, METH_NOARGS, morphos_getuid__doc__},
    {"getlogin",    morphos_getlogin, METH_NOARGS, morphos_getlogin__doc__},
#ifdef HAVE_PLOCK
    {"plock",   morphos_plock, METH_VARARGS, morphos_plock__doc__},
#endif /* HAVE_PLOCK */
    {"popen",   morphos_popen, METH_VARARGS, morphos_popen__doc__},
    {"setuid",  morphos_setuid, METH_VARARGS, morphos_setuid__doc__},
    {"setreuid",    morphos_setreuid, METH_VARARGS, morphos_setreuid__doc__},
    {"setregid",    morphos_setregid, METH_VARARGS, morphos_setregid__doc__},
    {"setgid",  morphos_setgid, METH_VARARGS, morphos_setgid__doc__},
    {"setgroups",   morphos_setgroups, METH_VARARGS, morphos_setgroups__doc__},
    {"setsid",  morphos_setsid, METH_NOARGS, morphos_setsid__doc__},
    {"open",    morphos_open, METH_VARARGS, morphos_open__doc__},
    {"close",   morphos_close, METH_VARARGS, morphos_close__doc__},
#ifdef PORTED
    {"dup",     morphos_dup, METH_VARARGS, morphos_dup__doc__},
    {"dup2",    morphos_dup2, METH_VARARGS, morphos_dup2__doc__},
#endif /* PORTED */
    {"lseek",   morphos_lseek, METH_VARARGS, morphos_lseek__doc__},
    {"read",    morphos_read, METH_VARARGS, morphos_read__doc__},
    {"write",   morphos_write, METH_VARARGS, morphos_write__doc__},
    {"fstat",   morphos_fstat, METH_VARARGS, morphos_fstat__doc__},
    {"fdopen",  morphos_fdopen, METH_VARARGS, morphos_fdopen__doc__},
    {"isatty",  morphos_isatty, METH_VARARGS, morphos_isatty__doc__},
#ifdef HAVE_MKFIFO
    {"mkfifo",  morphos_mkfifo, METH_VARARGS, morphos_mkfifo__doc__},
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
    {"mknod",   morphos_mknod, METH_VARARGS, morphos_mknod__doc__},
#endif
#ifdef HAVE_DEVICE_MACROS
    {"major",   morphos_major, METH_VARARGS, morphos_major__doc__},
    {"minor",   morphos_minor, METH_VARARGS, morphos_minor__doc__},
    {"makedev", morphos_makedev, METH_VARARGS, morphos_makedev__doc__},
#endif
#ifdef HAVE_FTRUNCATE
    {"ftruncate",   morphos_ftruncate, METH_VARARGS, morphos_ftruncate__doc__},
#endif
#ifdef HAVE_PUTENV
    {"putenv",  morphos_putenv, METH_VARARGS, morphos_putenv__doc__},
#endif
#ifdef HAVE_UNSETENV
    {"unsetenv",    morphos_unsetenv, METH_VARARGS, morphos_unsetenv__doc__},
#endif
#ifdef HAVE_STRERROR
    {"strerror",    morphos_strerror, METH_VARARGS, morphos_strerror__doc__},
#endif
#ifdef HAVE_FCHDIR
    {"fchdir",  morphos_fchdir, METH_O, morphos_fchdir__doc__},
#endif
#ifdef HAVE_FSYNC
    {"fsync",       morphos_fsync, METH_O, morphos_fsync__doc__},
#endif
#ifdef HAVE_FDATASYNC
    {"fdatasync",   morphos_fdatasync,  METH_O, morphos_fdatasync__doc__},
#endif
#ifdef HAVE_FSTATVFS
    {"fstatvfs",    morphos_fstatvfs, METH_VARARGS, morphos_fstatvfs__doc__},
#endif
#ifdef HAVE_STATVFS
    {"statvfs", morphos_statvfs, METH_VARARGS, morphos_statvfs__doc__},
#endif
    {"tmpfile", morphos_tmpfile, METH_NOARGS, morphos_tmpfile__doc__},
    {"tmpnam",  morphos_tmpnam, METH_NOARGS, morphos_tmpnam__doc__},
    {"abort",   morphos_abort, METH_NOARGS, morphos_abort__doc__},
#ifdef HAVE_GETLOADAVG
    {"getloadavg",  morphos_getloadavg, METH_NOARGS, morphos_getloadavg__doc__},
#endif

    /* Morphos Specifics */
    {"urandom", morphos_urandom, METH_VARARGS, morphos_urandom__doc__},
    {"AddPart", morphos_AddPart, METH_VARARGS, morphos_AddPart__doc__},
    {"getosfh", morphos_GetOSFileHandle, METH_VARARGS, morphos_GetOSFileHandle__doc__},
    {"uniqueid", morphos_uniqueid, METH_NOARGS, NULL},

    {NULL,      NULL}        /* Sentinel */
};//-

//+ ins()
static int
ins(PyObject *module, char *symbol, long value)
{
        return PyModule_AddIntConstant(module, symbol, value);
}//-

//+ all_ins()
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
#ifdef WCONTINUED
        if (ins(d, "WCONTINUED", (long)WCONTINUED)) return -1;
#endif
#ifdef WNOHANG
        if (ins(d, "WNOHANG", (long)WNOHANG)) return -1;
#endif
#ifdef WUNTRACED
        if (ins(d, "WUNTRACED", (long)WUNTRACED)) return -1;
#endif

        // Supported
        if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
        if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
        if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
        if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
        if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
        if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
        if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;

        // Defined but not handled
        if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
        if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;


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

#ifdef HAVE_SPAWNV
    if (ins(d, "P_WAIT", (long)_P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)_P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)_OLD_P_OVERLAY)) return -1;
    if (ins(d, "P_NOWAITO", (long)_P_NOWAITO)) return -1;
    if (ins(d, "P_DETACH", (long)_P_DETACH)) return -1;
#endif

        return 0;
}//-

#define INITFUNC initmorphos
#define MODNAME "morphos"

//+ INITFUNC()
PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m, *d, *dglobalv, *dlocalv, *dbothv, *daliases;

    m = Py_InitModule3(MODNAME, morphos_methods, morphos__doc__);
    if (NULL == m) return;

    d = PyModule_GetDict(m);

    /* Initialize environment dictionaries */
    if(!convertenviron(&dglobalv, &dlocalv, &dbothv, &daliases))
        Py_FatalError("can't read environment");

    if (PyDict_SetItemString(d, "environ", dbothv) != 0)
        Py_FatalError("can't define morphos.environ");
    Py_DECREF(dbothv);

    if (PyDict_SetItemString(d, "globalvars", dglobalv) != 0)
        Py_FatalError("can't define morphos.globalvars");
    Py_DECREF(dglobalv);

    if (PyDict_SetItemString(d, "shellvars", dlocalv) != 0)
        Py_FatalError("can't define morphos.shellvars");
    Py_DECREF(dlocalv);

    if (PyDict_SetItemString(d, "shellaliases", daliases) != 0)
        Py_FatalError("can't define morphos.shellaliases");
    Py_DECREF(daliases);

    if (all_ins(m))
        return;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

#ifdef HAVE_PUTENV
    if (morphos_putenv_garbage == NULL)
        morphos_putenv_garbage = PyDict_New();
#endif

    stat_result_desc.name = MODNAME ".stat_result";
    stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
    stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
    stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
    PyStructSequence_InitType(&StatResultType, &stat_result_desc);
    structseq_new = StatResultType.tp_new;
    StatResultType.tp_new = statresult_new;

    statvfs_result_desc.name = MODNAME ".statvfs_result";
    PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);

    Py_INCREF((PyObject*) &StatResultType);
    PyModule_AddObject(m, "stat_result", (PyObject*) &StatResultType);
    
    Py_INCREF((PyObject*) &StatVFSResultType);
    PyModule_AddObject(m, "statvfs_result", (PyObject*) &StatVFSResultType);
}//-
