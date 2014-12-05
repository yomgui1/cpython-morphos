#ifndef Py_PYTHON_H
#define Py_PYTHON_H
/* Since this is a "meta-include" file, no #ifdef __cplusplus / extern "C" { */

/* Include nearly all Python header files */

#include "patchlevel.h"
#include "pyconfig.h"
#include "pymacconfig.h"

#include <limits.h>

#ifndef UCHAR_MAX
#error "Something's broken.  UCHAR_MAX should be defined in limits.h."
#endif

#if UCHAR_MAX != 255
#error "Python's source code assumes C's unsigned char is an 8-bit type."
#endif

#if defined(__sgi) && defined(WITH_THREAD) && !defined(_SGI_MP_SOURCE)
#define _SGI_MP_SOURCE
#endif

#include <stdio.h>
#ifndef NULL
#   error "Python.h requires that stdio.h define NULL."
#endif

#include <string.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* For size_t? */
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

/* CAUTION:  Build setups should ensure that NDEBUG is defined on the
 * compiler command line when building Python in release mode; else
 * assert() calls won't be removed.
 */
#include <assert.h>

#include "pyport.h"
#include "pymacro.h"

#include "pyatomic.h"

/* Debug-mode build with pymalloc implies PYMALLOC_DEBUG.
 *  PYMALLOC_DEBUG is in error if pymalloc is not in use.
 */
#if defined(Py_DEBUG) && defined(WITH_PYMALLOC) && !defined(PYMALLOC_DEBUG)
#define PYMALLOC_DEBUG
#endif
#if defined(PYMALLOC_DEBUG) && !defined(WITH_PYMALLOC)
#error "PYMALLOC_DEBUG requires WITH_PYMALLOC"
#endif
#include "pymath.h"
#include "pytime.h"
#include "pymem.h"

#include "object.h"
#include "objimpl.h"
#include "typeslots.h"
#include "pyhash.h"

#include "pydebug.h"

#include "bytearrayobject.h"
#include "bytesobject.h"
#include "unicodeobject.h"
#include "longobject.h"
#include "longintrepr.h"
#include "boolobject.h"
#include "floatobject.h"
#include "complexobject.h"
#include "rangeobject.h"
#include "memoryobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "enumobject.h"
#include "setobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"
#include "pycapsule.h"
#include "traceback.h"
#include "sliceobject.h"
#include "cellobject.h"
#include "iterobject.h"
#include "genobject.h"
#include "descrobject.h"
#include "warnings.h"
#include "weakrefobject.h"
#include "structseq.h"
#include "namespaceobject.h"

#include "codecs.h"
#include "pyerrors.h"

#include "pystate.h"

#include "pyarena.h"
#include "modsupport.h"
#include "pythonrun.h"
#include "ceval.h"
#include "sysmodule.h"
#include "intrcheck.h"
#include "import.h"

#include "abstract.h"
#include "bltinmodule.h"

#include "compile.h"
#include "eval.h"

#include "pyctype.h"
#include "pystrtod.h"
#include "pystrcmp.h"
#include "dtoa.h"
#include "fileutils.h"
#include "pyfpe.h"

#if defined(__MORPHOS__)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#include <frameobject.h>
#include <libraries/python34_gvars.h>
extern struct Library *PythonBase;
#endif

/* From python_gvars.c */
struct PyMorphOS_GVar_STRUCT;
PyAPI_FUNC(void) PyMorphOS_InitGVars( struct PyMorphOS_GVar_STRUCT * );

/* From morphos.c */
PyAPI_FUNC(int) PyMorphOS_SetConfigA(int, struct TagItem *);
PyAPI_FUNC(void) PyMorphOS_Term( void );
PyAPI_FUNC(int) PyMorphOS_HandleArgv(int *, char ***);
PyAPI_FUNC(void *) PyMorphOS_GetGVars( void );
PyAPI_FUNC(size_t) PyMorphOS_GetFullPath( const char *path, char *buffer, size_t size );
PyAPI_FUNC(size_t) PyMorphOS_GetFullPathWide( const wchar_t *path, wchar_t *buffer, size_t size );
PyAPI_FUNC(int) PyMorphOS_AddTermFunc( void (*func)(void), const char *name );
PyAPI_FUNC(void *) PyMorphOS_InitThread(void);
PyAPI_FUNC(void) PyMorphOS_TermThread(void);

#include <utility/tagitem.h> /* For TAG_USER */
#define PYMOSATTR_DUMMY         (TAG_USER|0xa68f0000)
#define PYMOSATTR_GVARS_STORAGE (PYMOSATTR_DUMMY+0)
#define PYMOSATTR_EXIT_FUNC     (PYMOSATTR_DUMMY+2)
#define PYMOSATTR__EXIT_FUNC    (PYMOSATTR_DUMMY+3)
#define PYMOSATTR_STDIN         (PYMOSATTR_DUMMY+4)
#define PYMOSATTR_STDOUT        (PYMOSATTR_DUMMY+5)
#define PYMOSATTR_STDERR        (PYMOSATTR_DUMMY+6)
#define PYMOSATTR_MALLOC_FUNC   (PYMOSATTR_DUMMY+7)
#define PYMOSATTR_FREE_FUNC     (PYMOSATTR_DUMMY+8)

#ifdef __cplusplus
}
#endif

#endif /* __MORPHOS__ */

#endif /* !Py_PYTHON_H */
