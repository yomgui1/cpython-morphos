
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "importdl.h"

#include "morphos.h"

#include <dlfcn.h>

#define LEAD_UNDERSCORE ""

extern unsigned long __get_handle(int fd);

const char *_PyImport_DynLoadFiletab[] = {
	".pym",
	NULL,
};

static struct {
	BPTR amiga_lock;
	void *handle;
} handles[128];
static int nhandles = 0;

static void morphos_close_handles(void)
{
	int i;

	for (i = 0; i < nhandles; i++) {
		dlclose(handles[i].handle);
		if (handles[i].amiga_lock)
			UnLock(handles[i].amiga_lock);
	}
}

dl_funcptr _PyImport_GetDynLoadFunc(const char *shortname, const char *pathname, FILE *fp)
{
	dl_funcptr p;
	void *handle;
	char funcname[258];
	int dlopenflags=RTLD_GLOBAL;

	BOOL (*__PyMorphOS_InitModule)(APTR);
	void (*__PyMorphOS_TermModule)(void);

	if (nhandles == 0)
		PyMorphOS_AddTermFunc(morphos_close_handles, "morphos_close_handles");

	PyOS_snprintf(funcname, sizeof(funcname),
				LEAD_UNDERSCORE "PyInit_%.200s", shortname);

	if (fp != NULL) {
		/* Since RAM filesystem on MorphOS returns alwas the same values for st_dev and st_ino for any files.
		 * usage of fstat() is not possible in any cases on MorphOS.
		 * So I'm using the .ino field of handles strucuture to store a SHARED Lock on files.
		 */
		int i;
		BPTR amiga_lock = DupLockFromFH(__get_handle(fileno(fp)));

		if (Py_VerboseFlag)
			PySys_WriteStderr("%s: lock=%p\n", shortname, amiga_lock);

		if (!amiga_lock)
		{
			PyErr_SetString(PyExc_ImportError, "unable to obtain Lock on module");
			return NULL;
		}

		for (i = 0; i < nhandles; i++) {
			if (SameLock(amiga_lock, handles[i].amiga_lock) == LOCK_SAME) {
				if (Py_VerboseFlag)
					PySys_WriteStderr("%s: same lock found at nhandles=%u (lock=%p)\n", shortname, i, handles[i].amiga_lock);
				return (dl_funcptr) dlsym(handles[i].handle, funcname);
			}
		}
		if (nhandles < 128) {
			handles[nhandles].amiga_lock = amiga_lock;
			if (Py_VerboseFlag)
				PySys_WriteStderr("%s: saved at nhandles=%u\n", shortname, nhandles);
		}
	}

	if (Py_VerboseFlag)
		PySys_WriteStderr("dlopen(\"%s\", %x);\n", pathname, dlopenflags);

	handle = dlopen(pathname, dlopenflags);

	if (handle == NULL) {
		const char *error = dlerror();
		if (error == NULL)
			error = "unknown dlopen() error";
		PyErr_SetString(PyExc_ImportError, error);
		return NULL;
	}

	__PyMorphOS_InitModule = dlsym(handle, "__PyMorphOS_InitModule");
	if (!__PyMorphOS_InitModule) {
		PySys_WriteStderr("No __PyMorphOS_InitModule() function in \"%s\"? Please update the module!\n", pathname);
		dlclose(handle);
	}

	/* Terminate the initialization of the module.
	** This call may return 0 in case of error.
	*/
	if (!__PyMorphOS_InitModule(PythonBase)) {
		dlclose(handle);
		return NULL;
	}

	__PyMorphOS_TermModule = dlsym(handle, "__PyMorphOS_TermModule");
	if (NULL != __PyMorphOS_TermModule)
		PyMorphOS_AddTermFunc((APTR) __PyMorphOS_TermModule, "__PyMorphOS_TermModule");

	if (fp != NULL && nhandles < 128)
	{
		handles[nhandles++].handle = handle;
	}

	p = (dl_funcptr) dlsym(handle, funcname);
	if (Py_VerboseFlag)
		PySys_WriteStderr("%s: symbol %s @ %p\n", shortname, funcname, p);
	return p;
}
