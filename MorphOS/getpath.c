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
#define LANDMARK "os.py"
#endif

#ifndef PREFIX
#define PREFIX "SYS:"
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
#define FROMBUILD_PYTHONPATH                    \
    "Lib;"                                      \
    "Lib/plat-morphos"
#endif


/*
** Private Variables
*/

static char prefix[MAXPATHLEN+1] = {0};
static char exec_prefix[MAXPATHLEN+1] = {0};
static char fullpathprogname[MAXPATHLEN+1] = {0};
static char module_search_path[MAXPATHLEN+1] = {0};


/*
** Private Functions
*/

//+ reduce
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
//-
//+ isfile
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
//-
#if 0
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
#endif

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

static void
calculate_path(void)
{
    BOOL build = FALSE;
    char *delim;
    ULONG len;

    /* check binary launch dir */
    strncpy(prefix, gProgDir, MAXPATHLEN);
    AddPart(prefix, "pyconfig.h", MAXPATHLEN+1);
    build = isfile(prefix);

    /* computing <prefix> & <exec_prefix> */
    if (build) {
        reduce(prefix);
        strncpy(exec_prefix, prefix, MAXPATHLEN);
    } else {
        char *home = Py_GetPythonHome();

        /* Try <env PYTHONHOME> */
        if (home) {
            /* take only the first path given */
            delim = strchr(home, DELIM);
            if (delim)
                len = delim - prefix;
            else
                len = strlen(home);

            if (MAXPATHLEN < len) {
                fprintf(stderr, "Given Python home path is too long.\n");
                fprintf(stderr, "Using default static one: '%s'.\n", PREFIX);
                home = PREFIX;
                len = strlen(PREFIX);
            }

            strncpy(prefix, home, len);
            prefix[len] = '\0';
            strcpy(exec_prefix, prefix);
        } else {
            strncpy(prefix, PREFIX, MAXPATHLEN);
            strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
        }
    }

    /* check prefix/exec_prefix */
    if (!isdir(prefix)) {
        if (!Py_FrozenFlag)
            fprintf(stderr, "Could not find platform independent libraries <prefix>\n");

        strncpy(prefix, PREFIX, MAXPATHLEN);
    }

    if (!isdir(exec_prefix)) {
        if (!Py_FrozenFlag)
            fprintf(stderr, "Could not find platform dependent libraries <exec_prefix>\n");

        strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
    }

    if ((!isdir(prefix) || !isdir(exec_prefix)) && !Py_FrozenFlag)
        fprintf(stderr, "Consider setting $PYTHONHOME to <prefix>[;<exec_prefix>]\n");

    /* computing paths */
    if (build)
        _build_search_paths(module_search_path, MAXPATHLEN, prefix, FROMBUILD_PYTHONPATH);
    else {
        _build_search_paths(module_search_path, MAXPATHLEN, "PROGDIR:Libs", PYTHONPATHS);
        _build_search_paths(module_search_path, MAXPATHLEN, "Libs:", PYTHONPATHS);
    }

    DPRINT("module_search_path is '%s'\n", module_search_path);
    DPRINT("<prefix> is '%s'\n", prefix);
    DPRINT("<exec_prefix> is '%s'\n", exec_prefix);
}


/*
** Public Functions
*/

//+ Py_GetPath
char *
Py_GetPath(void)
{
    if (!module_search_path[0])
        calculate_path();
    return module_search_path;
}
//-
//+ Py_GetPrefix
char *
Py_GetPrefix(void)
{
    if (!module_search_path[0])
        calculate_path();
    return prefix;
}
//-
//+ Py_GetExecPrefix
char *
Py_GetExecPrefix(void)
{
    if (!module_search_path[0])
        calculate_path();
    return exec_prefix;
}
//-
//+ Py_GetProgramFullPath
char *
Py_GetProgramFullPath(void)
{
    if (fullpathprogname[0] == '\0') {
        strcpy(fullpathprogname, gProgDir);
        AddPart(fullpathprogname, gProgName, MAXPATHLEN+1);
    }

    return fullpathprogname;
}
//-
