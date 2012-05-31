/*******************************************************************************
 *** File: config.c
 *** Author: Guillaume Roguez (aka Yomgui)
 *** Date (YYYY/MM/DD): 20041213
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
 ***  + insert 'extern void init<module>(void);' before MARKER 1
 ***  + insert '{"<module>", initmodule},' before MARKER 2
 ***
 ***
 *** History:
 ***
 *** Date     | Author       | Desciption of modifications
 *** ---------|--------------|--------------------------------------------------
 *** 20041213 | Yomgui       | Initial Release
 *** 20041216 | Yomgui       | Add modules:
 ***          |              | arraymodule, binascii, cmath, zipimport
 *** 20041222 | Yomgui       | Return to a basic modules configuration
 *** 20041223 | Yomgui       | Add modules: cPickle, structmodule, operator
 ***          |              | _codec, unicodedata.
 *** 20041223 | Yomgui       | Add modules: sha, md5, _weakref, array, parser,
 ***          |              | collections, crypt, datetime.
 *** 20050112 | Yomgui       | complete modification of builins modules list.
 *** 20050303 | Yomgui       | Remove all unneed static modules.
 ***          |              | Add datetimemodule.
 *** 20050501 | Yomgui       | Add all missing extra modules.
 *** 20070205 | Yomgui       | Add thread.
 ***
 *******************************************************************************
 */

/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

/* Base modules */
extern void initthread(void);
extern void initsignal(void);
extern void initerrno(void);
extern void init_sre(void);
extern void initzipimport(void);
extern void init_symtable(void);

/* Host modules */
extern void initmorphos(void);
extern void inittime(void);
extern void initdatetime(void);

/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);
extern void initgc(void);

extern void initcollections(void);
extern void initcStringIO(void);
extern void init_random(void);
extern void init_codecs(void);

struct _inittab _PyImport_Inittab[] = {

    /* Base modules */
    {"thread",      initthread},
    {"signal",      initsignal},
    {"errno",       initerrno},
    {"_sre",        init_sre},
    {"zipimport",   initzipimport},
    {"_symtable",   init_symtable},

    /* Host modules */
    {"morphos",     initmorphos},
    {"time",        inittime},
    {"datetime",    initdatetime},

    {"collections", initcollections},
    {"cStringIO",   initcStringIO},
    {"_random",     init_random},
    {"_codecs",     init_codecs},

/* -- ADDMODULE MARKER 2 -- */

    /* This module lives in marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in import.c */
    {"imp", initimp},

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"sys", NULL},
    {"exceptions", NULL},

    /* This lives in gcmodule.c */
    {"gc", initgc},

    /* Sentinel */
    {NULL, NULL}
};
