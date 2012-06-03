#include <Python.h>

/* Stack checking: Most of basic python scripts need a minimum of a 32kB stack size
 * to run, but it can grow up fast specially when for recursive designs.
 * So we'll set a minimum of 100kB.
 */

unsigned long __stack = 200 * 1024;
const unsigned char *version = "$VER: python-" VERSION " " LIBVERSION "." LIBREVISION " (" BUILDDATE ") Python Interpreter";

int main(int argc, char **argv)
{
    int res;

    /* argv will be created if we have been launched from WB */
    res = PyMorphOS_HandleArgv(&argc, &argv);
    if (res) return res;

    res = Py_Main(argc, argv);
    return res == -1 ? RETURN_FAIL : res;
}
