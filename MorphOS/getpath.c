/*******************************************************************************
 *** $Id$
 ***
 *** Return the initial module search path.
 ***
 */

/* Search in some common locations for the associated Python libraries.
 *
 * Two directories must be found, the platform independent directory
 * (prefix), containing the common .py and .pyc files, and the platform
 * dependent directory (exec_prefix), containing the shared library
 * modules.  Note that prefix and exec_prefix can be the same directory,
 * but for some installations, they are different.
 *
 * Py_GetPath() carries out separate searches for prefix and exec_prefix.
 * Each search tries a number of different locations until a ``landmark''
 * file or directory is found.  If no prefix or exec_prefix is found, a
 * warning message is issued and the preprocessor defined PREFIX and
 * EXEC_PREFIX are used (even though they will not work); python carries on
 * as best as is possible, but most imports will fail.
 *
 * Then module_search_path is the concatenation of following paths:
 *      <env PYTHONPATH>
 *      <prefix>/libs/python<version>.zip
 *      <prefix>/libs/python<version> (builtin PYTHONPATH)
 *      <prefix>/libs/python<version>/lib-dynload (builtin PYTHONPATH)
 */

/* AmigaOS specifics:
 *
 * Py_GetPath() will return following paths:
 *
 *      <env PYTHONPATH>
 * if the bin is launched from the build dir:
 *      <srcdir>/Lib
 *      <srcdir>/Lib/plat-morphos
 * else
 *      LIBS:python<VERSION_SHORT>.zip
 *      LIBS:python<VERSION_MAJOR><VERSION_MINOR>
 *      LIBS:python<VERSION_MAJOR><VERSION_MINOR>/plat-morphos
 *      LIBS:python<VERSION_MAJOR><VERSION_MINOR>/lib-dynload
 *      PROGDIR:Libs/python<VERSION_SHORT>.zip
 *      PROGDIR:Libs/python<VERSION_MAJOR><VERSION_MINOR>
 *      PROGDIR:Libs/python<VERSION_MAJOR><VERSION_MINOR>/plat-morphos
 *      PROGDIR:Libs/python<VERSION_MAJOR><VERSION_MINOR>/lib-dynload
 *
 * Py_GetPrefix()/Py_GetExecPrefix() will return the "<srcdir>"
 * if the binary is launched from the build dir,
 * else it's value of $PYTHONHOME if defined or SYS: if not.
 */


/*
** Project Includes
*/

#include "Python.h"
#include "osdefs.h"
#include "morphos.h"


/*
** System Includes
*/

#include <proto/dos.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


/*
** Private Macros and Definitions
*/

#ifndef LANDMARK
#define LANDMARK L"os.py"
#endif

#ifndef PREFIX
#define PREFIX L"SYS:"
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX PREFIX
#endif

#ifndef PYTHONPATHS
#define PYTHONPATHS \
    "python" VERSION "/plat-morphos;"          \
    "python" VERSION ";"                       \
    "python" VERSION "/lib-dynload;"           \
    "python" VERSION_SHORT ".zip"
#endif

#ifndef FROMBUILD_PYTHONPATH
#define FROMBUILD_PYTHONPATH L"Lib"
#endif


/*
** Private Variables
*/

static wchar_t prefix[MAXPATHLEN+1] = {0};
static wchar_t exec_prefix[MAXPATHLEN+1] = {0};
static wchar_t progpath[MAXPATHLEN+1] = {0};
static wchar_t *module_search_path = NULL;
static int module_search_path_malloced = 0;

/*
** Private Functions
*/

static void
reduce(char *dir)
{
    size_t i = strlen(dir);

    while ((i > 0) && (dir[i] != SEP) && (dir[i] != ':')) --i;

    /* don't strip the ':' ! */
    if (dir[i] == ':')
        i++;

    dir[i] = '\0';
}

static int
isfile(char *filename)
{
    struct stat buf;
    LONG res;

    res = stat(filename, &buf);
    if ((res >= 0) && S_ISREG(buf.st_mode))
        res = TRUE;
    else
        res = FALSE;

    return res;
}

/*  Assumes 'filename' MAXPATHLEN+1 bytes long -
 * may extend 'filename' by one character.
 */
static int
ismodule(char *filename)    /* Is module -- check for .pyc/.pyo too */
{
    if (isfile(filename))
        return 1;

    /* Check for the compiled version of prefix. */
    if (strlen(filename) < MAXPATHLEN) {
        strcat(filename, Py_OptimizeFlag ? "o" : "c");
        if (isfile(filename))
            return 1;
    }
    return 0;
}

static LONG
isdir(char *path)
{
    struct stat buf;
    LONG res;

    res = stat(path, &buf);
    if ((res >= 0) && S_ISDIR(buf.st_mode))
        res = TRUE;
    else
        res = FALSE;

    return res;
}

/* Check if 'path' is like 'device:dir1/dir2/...'.
** Assume that the path is well formed.
** Stop at DELIM.
*/
static int
isabsolute(char *path)
{
    static char delimiter[3] = {DELIM, ':', '\0'};
    char * p;
    int result;

    if (*path != '\0')
    {
        p = path + strcspn(path, delimiter);
        result = (*p == ':');
    }
    else
        result = FALSE;

    DPRINT("path '%s' is%s absolute\n", path, result?"":" not");
    return result;
}

static void
_build_search_paths(char *search_path, size_t size, const char *path_prefix, char *paths)
{
	char temp[MAXPATHLEN];
	char *delim, *delim2;
	ULONG len, prefix_len;
	BPTR lock;
	
	if (path_prefix)
		prefix_len = strlen(path_prefix);
	else
		prefix_len = 0;

	len = 0;
	for (;;) {
		delim = strchr(paths, ';');
		if (delim)
			len = delim - paths;
		else
			len = strlen(paths);

		strncpy(temp, path_prefix, sizeof(temp));
		AddPart(temp, paths, sizeof(temp));
		delim2 = strchr(temp, ';');
		if (delim2)
			delim2[0] = '\0';
		
		lock = Lock(temp, SHARED_LOCK);
		if (lock) {
			UnLock(lock);
			if (search_path[0] != '\0')
				strncat(search_path, ";", size);
			strncat(search_path, temp, size);
		}

		if (!delim)
			break;
			
		paths += len + 1;
	}
	
	DPRINT("module_search_path is:\n%s\n", search_path);
}

static void
calculate_path(void)
{
    BOOL build = FALSE;
    size_t r;
    char _prefix[MAXPATHLEN+1];

    /* check binary launch dir */
    strncpy(_prefix, gProgDir, MAXPATHLEN);
    AddPart(_prefix, "pyconfig.h", MAXPATHLEN+1);
    build = isfile(_prefix);

    /* computing <prefix> & <exec_prefix> */
    if (build) {
        reduce(_prefix);
        
        r = mbstowcs(prefix, _prefix, MAXPATHLEN);
        if (r == (size_t)-1 || r > MAXPATHLEN) {
            wcsncpy(prefix, (wchar_t *)PREFIX, MAXPATHLEN);
            wcsncpy(exec_prefix, (wchar_t *)EXEC_PREFIX, MAXPATHLEN);
        }
    } else {
        wcsncpy(prefix, (wchar_t *)PREFIX, MAXPATHLEN);
        wcsncpy(exec_prefix, (wchar_t *)EXEC_PREFIX, MAXPATHLEN);
    }

    if (build) {
        module_search_path = FROMBUILD_PYTHONPATH;
        module_search_path_malloced = 0;
    } else {
        char tmp_search_path[MAXPATHLEN+1];
        
        tmp_search_path[0] = '\0';
        _build_search_paths(tmp_search_path, sizeof(tmp_search_path), "Libs", PYTHONPATHS);
        _build_search_paths(tmp_search_path, sizeof(tmp_search_path), "LIBS:", PYTHONPATHS);
        
        module_search_path = _Py_char2wchar(tmp_search_path, NULL);
        module_search_path_malloced = 1;
    }
}

/*
** Public Functions
*/

void
Py_SetPath(const wchar_t *path)
{
    if (module_search_path != NULL) {
        if (module_search_path_malloced)
            PyMem_Free(module_search_path);
        module_search_path = NULL;
        module_search_path_malloced = 0;
    }
    if (path != NULL) {
        extern wchar_t *Py_GetProgramName(void);
        wchar_t *prog = Py_GetProgramName();
        wcsncpy(progpath, prog, MAXPATHLEN);
        exec_prefix[0] = prefix[0] = L'\0';
        module_search_path = PyMem_Malloc((wcslen(path) + 1) * sizeof(wchar_t));
        module_search_path_malloced = 1;
        if (module_search_path != NULL)
            wcscpy(module_search_path, path);
    }
}

wchar_t *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

wchar_t *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

wchar_t *
Py_GetExecPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return exec_prefix;
}

wchar_t *
Py_GetProgramFullPath(void)
{
    if (progpath[0] == L'\0') {
        char path[MAXPATHLEN+1];
        size_t r;

        strcpy(path, gProgDir);
        AddPart(path, gProgName, sizeof(path));

        r = mbstowcs(progpath, path, MAXPATHLEN);
        if (r == (size_t)-1 || r > MAXPATHLEN) {
            extern wchar_t *Py_GetProgramName(void);
            wcsncpy(progpath, (wchar_t *)L"PROGDIR:", MAXPATHLEN);
            wcscat(progpath, Py_GetProgramName());
        }
    }

    return progpath;
}
