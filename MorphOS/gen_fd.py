#!/usr/bin/env python
import os, sys, re

bin_filename = sys.argv[1]
if len(sys.argv) == 3:
    oldfd_filename = sys.argv[2]
else:
    oldfd_filename = None

apifunc_match = re.compile('\w+ .{7} .text.pyapi\W+\w+ \w+').match

# Removing varargs functions
apifunc_removed = "PyArg_ParseTuple PyArg_ParseTupleAndKeywords Py_BuildValue _Py_BuildValue_SizeT" \
                  " _PyArg_ParseTuple_SizeT _PyArg_ParseTupleAndKeywords_SizeT PyArg_UnpackTuple" \
                  " PyEval_CallFunction PyEval_CallMethod PySys_WriteStdout PySys_WriteStderr" \
                  " PyTuple_Pack PyErr_Format PyOS_snprintf " \
                  " PyObject_CallFunction _PyObject_CallFunction_SizeT" \
                  " PyObject_CallMethod _PyObject_CallMethod_SizeT" \
                  " PyObject_CallMethodObjArgs PyObject_CallFunctionObjArgs" \
                  " PyBytes_FromFormat PyUnicode_FromFormat"
apifunc_removed = set(apifunc_removed.split())

print "** Parsing binary '%s' ..." % bin_filename
with os.popen('objdump -t %s' % bin_filename) as f:
    apifunc = set(x.split()[-1] for x in f.xreadlines() if apifunc_match(x)) - apifunc_removed
print "** %lu Py functions found in the binary" % len(apifunc)

newfunc = sorted(apifunc)
print "** %lu API functions found after cleanup" % len(newfunc)

if oldfd_filename:
    print "** Parsing old FDs' '%s' ..." % oldfd_filename
    with open(oldfd_filename, 'Ur') as f:
        oldfds = [x[:x.find('(')] for x in f.xreadlines() if x[0] in '_P' or x.startswith('private')]
    print "** %lu functions found" % (len(oldfds) - oldfds.count('private'))

    print "** Removing old functions ..."
    cnt = 0
    for i,n in enumerate(oldfds):
        if n not in newfunc and n != 'private':
            oldfds[i] = "private"
            cnt += 1
            #print "Removed:", n
    print "** %lu functions removed" % cnt

    print "** Adding new functions ..."
    cnt = len(oldfds)
    oldfds += [n for n in newfunc if n not in oldfds]
    print "** %lu functions added" % (len(oldfds)-cnt)

    cnt = oldfds.count('private')
    assert (len(set(oldfds)) - (1 if cnt else 0)) == (len(oldfds)-cnt)
else:
    oldfds = newfunc

fd_filename = 'include/fd/python34_lib.fd'
print "** Creating '%s'" % fd_filename
with open(fd_filename, 'w') as f:
    f.write("##base _PythonBase\n##bias 30\n##public\n")
    f.writelines((x+'()(sysv, r12base)\n' if x != 'private' else "##private\nprivate()()\n##public\n") for x in oldfds)


clib_template = """#ifndef PYTHON_PROTOS_H
#define PYTHON_PROTOS_H

#include <Python.h>

#if 0 /* don't use it... it's already given by Python.h (it's just for cvinclude.pl) */
%s
#endif

#if defined(USE_INLINE_STDARG) && !defined(__STRICT_ANSI__)
#include <stdarg.h>
#define PyMorphOS_SetConfig(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	PyMorphOS_SetConfigA(__p0, (struct TagItem *)_tags);})
#endif

#endif /* PYTHON_PROTOS_H */
"""

specials = {
    # Functions with more than 8 parameters
    'PyCode_New' : """PyCodeObject * PyCode_New(int argcount, int kwonlyargcount,
        int nlocals, int stacksize, int flags,
        PyObject *code, PyObject *consts, PyObject *names,
        PyObject *varnames, PyObject *freevars, PyObject *cellvars,
        PyObject *filename, PyObject *name, int firstlineno,
        PyObject *lnotab)""",
    'PyFile_FromFd' : "PyObject * PyFile_FromFd(int, char *, char *, int, char *, char *, char *, int)",
    'PyParser_ParseFileFlags' : """node * PyParser_ParseFileFlags(FILE *, const char *,
        const char*, grammar *,
        int, char *, char *,
        perrdetail *, int)""",
    'PyParser_ParseFileFlagsEx' : """node * PyParser_ParseFileFlagsEx(FILE *, const char *,
        const char*, grammar *,
        int, char *, char *,
        perrdetail *, int *)""",
    'PyParser_ASTFromFile' : """struct _mod * PyParser_ASTFromFile(FILE *, const char *,
        const char*, int,
        char *, char *,
        PyCompilerFlags *, int *,
        PyArena *)""",
    }

clib_filename = 'include/clib/python34_protos.h'
print "** Creating '%s'" % clib_filename
with open(clib_filename, 'w') as f:
    data = []
    for n in oldfds:
        if n == 'private': continue;
        if n in specials:
            data.append('extern %s;' % specials[n])
        else:
            data.append('extern int %s (void);' % n)
    f.write(clib_template % '\n'.join(data))
