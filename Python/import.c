/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module definition and import implementation */

#include "allobjects.h"

#include "node.h"
#include "token.h"
#include "graminit.h"
#include "import.h"
#include "errcode.h"
#include "sysmodule.h"
#include "pythonrun.h"
#include "marshal.h"
#include "compile.h"
#include "eval.h"
#include "osdefs.h"

extern int verbose; /* Defined in pythonmain.c */

extern long getmtime(); /* Defined in posixmodule.c */

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#ifdef USE_DL
#ifdef SUN_SHLIB
#include <dlfcn.h>
typedef void (*dl_funcptr)();
#else
#include "dl.h"
#endif /* SUN_SHLIB */

extern char *argv0;
#endif

/* Magic word to reject .pyc files generated by other Python versions */

#define MAGIC 0x999902L /* Increment by one for each incompatible change */

static object *modules;

/* Forward */
static int init_builtin PROTO((char *));

/* Initialization */

void
initimport()
{
	if ((modules = newdictobject()) == NULL)
		fatal("no mem for dictionary of modules");
}

object *
get_modules()
{
	return modules;
}

object *
add_module(name)
	char *name;
{
	object *m;
	if ((m = dictlookup(modules, name)) != NULL && is_moduleobject(m))
		return m;
	m = newmoduleobject(name);
	if (m == NULL)
		return NULL;
	if (dictinsert(modules, name, m) != 0) {
		DECREF(m);
		return NULL;
	}
	DECREF(m); /* Yes, it still exists, in modules! */
	return m;
}

/* Suffixes used by find_module: */

#define PY_SUFFIX	".py"
#define PYC_SUFFIX	".pyc"
#ifdef USE_DL
#ifdef SUN_SHLIB
#define O_SUFFIX	"module.so"
#else
#define O_SUFFIX	"module.o"
#endif /* SUN_SHLIB */
#endif

/* This will search for a module named 'name' with the extension 'ext'
   and return it in 'namebuf' and return the mtime of each in 'mtime'.
   It returns a file pointer opened for 'mode' if successful, NULL if
   unsuccessful.
 */
static FILE *
find_module(name, ext, mode, namebuf, mtime)
	char *name;
	char *ext;
	char *mode;
	char *namebuf;
	long *mtime;
{
	object *path;
	FILE *fp;

	path = sysget("path");
	if (path == NULL || !is_listobject(path)) {
		/* No path -- at least try current directory */
		strcpy(namebuf, name);
		strcat(namebuf, ext);
		if ((fp = fopen(namebuf, mode)) == NULL)
			return NULL;
		*mtime = getmtime(namebuf);
		return fp;
	} else {
		int npath = getlistsize(path);
		int i;
		for (i = 0; i < npath; i++) {
			object *v = getlistitem(path, i);
			int len;
			if (!is_stringobject(v))
				continue;
			strcpy(namebuf, getstringvalue(v));
			len = getstringsize(v);
			if (len > 0 && namebuf[len-1] != SEP)
				namebuf[len++] = SEP;
			strcpy(namebuf+len, name);
			strcat(namebuf, ext);
			if ((fp = fopen(namebuf, mode)) == NULL)
				continue;
			*mtime = getmtime(namebuf);
			return fp;
		}
	}
	namebuf[0] = '\0';
	return NULL;
}

static object *
get_module(m, name, m_ret)
	/*module*/object *m;
	char *name;
	object **m_ret;
{
	codeobject *co = NULL;
	object *v, *d;
	FILE *fp, *fpc;
	node *n;
	int err;
	char namebuf[MAXPATHLEN+1];
	int namelen;
	long mtime;

#ifdef USE_DL
	if ((fpc = find_module(name, O_SUFFIX, "rb",
			       namebuf, &mtime)) != NULL) {
		char funcname[258];
		dl_funcptr p;
		D(fprintf(stderr, "Found %s\n", namebuf));
		fclose(fpc);
		sprintf(funcname, "init%s", name);
#ifdef SUN_SHLIB
		{
		  void *handle = dlopen (namebuf, 1);
		  p = (dl_funcptr) dlsym(handle, funcname);
		}
#else
		if (verbose)
			fprintf(stderr,
				"import %s # dynamically loaded from \"%s\"\n",
				name, namebuf);
		p =  dl_loadmod(argv0, namebuf, funcname);
#endif /* SUN_SHLIB */
		if (p == NULL) {
			err_setstr(SystemError,
			   "dynamic module does not define init function");
			return NULL;
		} else {
			(*p)();
			*m_ret = m = dictlookup(modules, name);
			if (m == NULL) {
				err_setstr(SystemError,
				   "dynamic module not initialized properly");
				return NULL;
			} else {
				D(fprintf(stderr,
					"module %s loaded!\n", name));
				INCREF(None);
				return None;
			}
		}
	}
	else
#endif
	if ((fpc = find_module(name, PYC_SUFFIX, "rb",
			      namebuf, &mtime)) != NULL) {
		long pyc_mtime;
		long magic;
		namebuf[(strlen(namebuf)-1)] = '\0';
		mtime = getmtime(namebuf);
		magic = rd_long(fpc);
		pyc_mtime = rd_long(fpc);
		if (mtime != -1 && mtime > pyc_mtime) {
			fclose(fpc);
			goto read_py;
		}
		if (magic == MAGIC) {
			v = rd_object(fpc);
			if (v == NULL || err_occurred() ||
			    !is_codeobject(v)) {
				err_clear();
				XDECREF(v);
			}
			else
				co = (codeobject *)v;
		}
		fclose(fpc);
		if (verbose) {
			if (co != NULL)
				fprintf(stderr,
			"import %s # precompiled from \"%s\"\n",
					name, namebuf);
			else {
				fprintf(stderr,
				"# invalid precompiled file \"%s\"\n",
					namebuf);
				goto read_py;
			}
		}
	}
	else {
read_py:
		if ((fp = find_module(name, PY_SUFFIX, "r",
				      namebuf, &mtime)) != NULL) {
			namelen = strlen(namebuf);
			if (co == NULL) {
				if (verbose)
					fprintf(stderr,
						"import %s # from \"%s\"\n",
						name, namebuf);
				err = parse_file(fp, namebuf, file_input, &n);
			} else
				err = E_DONE;
			fclose(fp);
			if (err != E_DONE) {
				err_input(err);
				return NULL;
			}
		}
		else {
			if (m == NULL) {
				sprintf(namebuf, "no module named %.200s",
					name);
				err_setstr(ImportError, namebuf);
			}
			else {
				sprintf(namebuf, "no source for module %.200s",
					name);
				err_setstr(ImportError, namebuf);
			}
			return NULL;
		}
	}
	if (m == NULL) {
		m = add_module(name);
		if (m == NULL) {
			freetree(n);
			return NULL;
		}
		*m_ret = m;
	}
	d = getmoduledict(m);
	if (co == NULL) {
		co = compile(n, namebuf);
		freetree(n);
		if (co == NULL)
			return NULL;
		/* Now write the code object to the ".pyc" file */
		namebuf[namelen] = 'c';
		namebuf[namelen+1] = '\0';
		fpc = fopen(namebuf, "wb");
		if (fpc != NULL) {
			wr_long(MAGIC, fpc);
			/* First write a 0 for mtime */
			wr_long(0L, fpc);
			wr_object((object *)co, fpc);
			if (ferror(fpc)) {
				/* Don't keep partial file */
				fclose(fpc);
				(void) unlink(namebuf);
			}
			else {
				/* Now write the true mtime */
				fseek(fpc, 4L, 0);
				wr_long(mtime, fpc);
				fflush(fpc);
				fclose(fpc);
			}
		}
	}
	v = eval_code(co, d, d, d, (object *)NULL);
	DECREF(co);
	return v;
}

static object *
load_module(name)
	char *name;
{
	object *m, *v;
	v = get_module((object *)NULL, name, &m);
	if (v == NULL)
		return NULL;
	DECREF(v);
	return m;
}

object *
import_module(name)
	char *name;
{
	object *m;
	int n;
	if ((m = dictlookup(modules, name)) == NULL) {
		if ((n = init_builtin(name)) || (n = init_frozen(name))) {
			if (n < 0)
				return NULL;
			if ((m = dictlookup(modules, name)) == NULL)
				err_setstr(SystemError,
					   "builtin module missing");
		}
		else {
			m = load_module(name);
		}
	}
	return m;
}

object *
reload_module(m)
	object *m;
{
	if (m == NULL || !is_moduleobject(m)) {
		err_setstr(TypeError, "reload() argument must be module");
		return NULL;
	}
	/* XXX Ought to check for builtin modules -- can't reload these... */
	return get_module(m, getmodulename(m), (object **)NULL);
}

void
doneimport()
{
	if (modules != NULL) {
		int pos;
		object *modname, *module;
		/* Explicitly erase all modules; this is the safest way
		   to get rid of at least *some* circular dependencies */
		pos = 0;
		while (mappinggetnext(modules, &pos, &modname, &module)) {
			if (is_moduleobject(module)) {
				object *dict;
				dict = getmoduledict(module);
				if (dict != NULL && is_dictobject(dict))
					mappingclear(dict);
			}
		}
		mappingclear(modules);
	}
	DECREF(modules);
	modules = NULL;
}


/* Initialize built-in modules when first imported */

static int
init_builtin(name)
	char *name;
{
	int i;
	for (i = 0; inittab[i].name != NULL; i++) {
		if (strcmp(name, inittab[i].name) == 0) {
			if (verbose)
				fprintf(stderr, "import %s # builtin\n",
					name);
			(*inittab[i].initfunc)();
			return 1;
		}
	}
	return 0;
}

extern struct frozen {
	char *name;
	char *code;
	int size;
} frozen_modules[];

int
init_frozen(name)
	char *name;
{
	struct frozen *p;
	codeobject *co;
	object *m, *d, *v;
	for (p = frozen_modules; ; p++) {
		if (p->name == NULL)
			return 0;
		if (strcmp(p->name, name) == 0)
			break;
	}
	if (verbose)
		fprintf(stderr, "import %s # frozen\n", name);
	co = (codeobject *) rds_object(p->code, p->size);
	if (co == NULL)
		return -1;
	if ((m = add_module(name)) == NULL ||
	    (d = getmoduledict(m)) == NULL ||
	    (v = eval_code(co, d, d, d, (object*)NULL)) == NULL) {
		DECREF(co);
		return -1;
	}
	DECREF(co);
	DECREF(v);
	return 1;
}
