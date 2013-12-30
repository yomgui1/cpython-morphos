#include "Python.h"
#include <locale.h>

unsigned long __stack = 200 * 1024;
const unsigned char *version = "$VER: python-" VERSION " " LIBVERSION "." LIBREVISION " (" BUILDDATE ") Python Interpreter";

static wchar_t*
char2wchar(char* arg)
{
    wchar_t *res;
    size_t argsize = strlen(arg);
    size_t count;
    unsigned char *in;
    wchar_t *out;
    mbstate_t mbs;

    if (argsize != (size_t)-1) {
	res = (wchar_t *)PyMem_Malloc((argsize+1)*sizeof(wchar_t));
	if (!res)
	    goto oom;
	count = mbstowcs(res, arg, argsize+1);
	if (count != (size_t)-1) {
	    wchar_t *tmp;
	    /* Only use the result if it contains no
	       surrogate characters. */
	    for (tmp = res; *tmp != 0 && (*tmp < 0xd800 || *tmp > 0xdfff); tmp++)
                ;
	    if (*tmp == 0)
		return res;
	}
	PyMem_Free(res);
    }
    /* Conversion failed. Fall back to escaping with surrogateescape. */

    /* Try conversion with mbrtwoc (C99), and escape non-decodable bytes. */

    /* Overallocate; as multi-byte characters are in the argument, the
       actual output could use less memory. */
    argsize = strlen(arg) + 1;
    res = PyMem_Malloc(argsize*sizeof(wchar_t));
    if (!res) goto oom;
    in = (unsigned char*)arg;
    out = res;
    memset(&mbs, 0, sizeof mbs);
    while (argsize) {
	size_t converted = mbrtowc(out, (char*)in, argsize, &mbs);
	if (converted == 0)
	    /* Reached end of string; null char stored. */
	    break;
	if (converted == (size_t)-2) {
	    /* Incomplete character. This should never happen,
	       since we provide everything that we have -
	       unless there is a bug in the C library, or I
	       misunderstood how mbrtowc works. */
	    fprintf(stderr, "unexpected mbrtowc result -2\n");
	    PyMem_Free(res);
            return NULL;
	}
	if (converted == (size_t)-1) {
	    /* Conversion error. Escape as UTF-8b, and start over
	       in the initial shift state. */
	    *out++ = 0xdc00 + *in++;
	    argsize--;
	    memset(&mbs, 0, sizeof mbs);
	    continue;
	}
	if (*out >= 0xd800 && *out <= 0xdfff) {
	    /* Surrogate character.  Escape the original
	       byte sequence with surrogateescape. */
	    argsize -= converted;
	    while (converted--)
		    *out++ = 0xdc00 + *in++;
	    continue;
	}
	/* successfully converted some bytes */
	in += converted;
	argsize -= converted;
	out++;
    }
    return res;
oom:
    fprintf(stderr, "out of memory\n");
    return NULL;
}

int main(int argc, char **argv)
{
    wchar_t **argv_copy;
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2;
    int i, res;
    char *oldloc;

    /* argv will be created if we have been launched from WB */
    res = PyMorphOS_HandleArgv(&argc, &argv);
    if (res) return res;

    argv_copy = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    argv_copy2 = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    
    if (!argv_copy || !argv_copy2) {
	fprintf(stderr, "out of memory\n");
	return RETURN_FAIL;
    }
    oldloc = strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
	argv_copy2[i] = argv_copy[i] = char2wchar(argv[i]);
	if (!argv_copy[i])
            return RETURN_FAIL;
    }
    setlocale(LC_ALL, oldloc);
    free(oldloc);
    res = Py_Main(argc, argv_copy);
    for (i = 0; i < argc; i++) {
	PyMem_Free(argv_copy2[i]);
    }
    PyMem_Free(argv_copy);
    PyMem_Free(argv_copy2);
    return res == -1 ? RETURN_FAIL : res;
}

