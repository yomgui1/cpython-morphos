/*******************************************************************************
 *** $Id:$
 ***
 ***
 *** Desciption:
 ***
 *** This C file contains all structures and code to initialise all builtins
 *** modules.
 ***
 ***
 *** Modification note:
 ***
 *** To add a new modules initialization:
 ***  + insert 'extern PyObject* PyInit_<module>(void);' before MARKER 1
 ***  + insert '{"<module>", PyInit_<module>},' before MARKER 2
 ***
 ***
 */

/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

/* Host modules */
extern PyObject* PyInit_morphos(void);
extern PyObject* PyInit_time(void);

/* No shared modules */
extern PyObject* PyInit__io(void);
extern PyObject* PyInit__thread(void);
extern PyObject* PyInit_signal(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit__sre(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* PyInit__functools(void);
extern PyObject* PyInit_zipimport(void);
extern PyObject* PyInit__symtable(void);
extern PyObject* PyInit__locale(void);
extern PyObject* PyInit__collections(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit__operator(void);
extern PyObject* PyInit_parser(void);
extern PyObject* PyInit_atexit(void);
extern PyObject* PyInit__stat(void);
extern PyObject* PyInit__tracemalloc(void);
extern PyObject* PyInit__struct(void);

/* -- ADDMODULE MARKER 1 -- */

extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit_imp(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit__ast(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);

struct _inittab _PyImport_Inittab[] = {

    /* Host modules */
    {"morphos",      PyInit_morphos},
    {"time",         PyInit_time},
    {"_thread",      PyInit__thread},
    {"signal",       PyInit_signal},

    /* No shared modules */
    {"errno",        PyInit_errno},
    {"_sre",         PyInit__sre},
    {"_codecs",      PyInit__codecs},
    {"_weakref",     PyInit__weakref},
    {"_functools",   PyInit__functools},
    {"operator",     PyInit__operator},
    {"_collections", PyInit__collections},
    {"itertools",    PyInit_itertools},
    {"_locale",      PyInit__locale},
    {"_io",          PyInit__io},
    {"zipimport",    PyInit_zipimport},
    {"_symtable",    PyInit__symtable},
    {"parser",       PyInit_parser},
    {"atexit",       PyInit_atexit},
    {"_stat",        PyInit__stat},
    {"_tracemalloc", PyInit__tracemalloc},
    {"_struct",      PyInit__struct},

/* -- ADDMODULE MARKER 2 -- */

    /* This module lives in marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in import.c */
    {"imp", PyInit_imp},

    /* This lives in Python/Python-ast.c */
    {"_ast", PyInit__ast},

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"builtins", NULL},
    {"sys", NULL},

    /* This lives in gcmodule.c */
    {"gc", PyInit_gc},

    /* This lives in _warnings.c */
    {"_warnings", _PyWarnings_Init},

    /* This lives in Objects/unicodeobject.c */
    {"_string", PyInit__string},

    /* Sentinel */
    {0, 0}
};
