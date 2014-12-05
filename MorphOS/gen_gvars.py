#!/usr/bin/env python

from __future__ import with_statement
import os, sys, re

newincludedir = sys.argv[1]

gvars_match = re.compile('[ \t]*PyAPI_DATA\(([^)]+)\)[ \t]+([^;]+);.*').match
# From 2.x
remove_gvars = 'PyExc_WindowsError PyExc_VMSError PyCmpWrapper_Type'.split()
remove_gvars += '_PySys_CheckInterval _PySys_ProfileFunc _PySys_TraceFunc _Py_RefTotal'.split()
# From 3.x
remove_gvars += ['PySortWrapper_Type', '_Py_HashSecret_Initialized', 'PyNullImporter_Type']

if not os.path.isdir(newincludedir):
    raise ValueError("'%s' is not a directory" % newincludedir)

print "** Parsing include directory '%s' ..." % newincludedir

d = {}
for item in os.listdir(newincludedir):
    if not item.lower().endswith('.h'): continue
    l = []
    with open(os.path.join(newincludedir, item), 'Ur') as f:
        for line in f.xreadlines():
            m = gvars_match(line)
            if m:
                t, v = map(str.strip, m.groups())
                if '(*' not in v:
                    v = map(str.strip, v.split(','))
                else:
                    v = [v]
                if len(v) == 1:
                    l.append("%s;%s" % (v[0], t))
                else:
                    for x in v:
                        l.append("%s;%s" % (x, t))
    if l: d[item] = l

gvars = sum(d.itervalues(), [])
print "** %lu global variables found" % len(gvars)

topdir = os.path.dirname(newincludedir)
srcdir = [ os.path.join(topdir, 'Objects'),
           os.path.join(topdir, 'Python') ]

d.clear()
for dirname in srcdir:
    print "** Parsing source directory '%s' ..." % dirname
    for item in os.listdir(dirname):
        if not item.lower().endswith('.c'): continue
        l = []
        with open(os.path.join(dirname, item), 'Ur') as f:
            for line in f.xreadlines():
                m = gvars_match(line)
                if m:
                    t, v = map(str.strip, m.groups())
                    if '(*' not in v:
                        v = map(str.strip, v.split(','))
                    else:
                        v = [v]
                    if len(v) == 1:
                        l.append("%s;%s" % (v[0], t))
                    else:
                        for x in v:
                            l.append("%s;%s" % (x, t))
        if l: d[item] = l
gvars_src = sum(d.itervalues(), [])
print "** %lu global variables found in sources" % len(gvars_src)

gvars += gvars_src


print "** Cleanup gvars ..."
l = set()
for i,x in enumerate(gvars):
    n,t = x.split(';')

    if n.endswith(']'):
        n = n[:n.find('[')]
        t += '*'
        gvars[i] = '%s;%s' % (n, t)
        print "Array    : %-65s (was '%s')" % (gvars[i], x)
        x = gvars[i]

    if n.startswith('*'):
        cnt = 0
        for c in n:
            if c != '*': break
            cnt += 1
        n = n[cnt:]
        t += '*' * cnt
        gvars[i] = '%s;%s' % (n, t)
        print "Stars    : %-65s (was '%s')" % (gvars[i], x)
        x = gvars[i]

    if n.startswith('('):
        p = n.find(')')
        t += "(*)"+n[p+1:].strip()
        n = n[2:p]
        gvars[i] = '%s;%s' % (n, t)
        print "FuncPtr  : %-65s (was '%s')" % (gvars[i], x)
        x = gvars[i]

    if n in l:
        print "Duplicate: %-65s (was '%s')" % (n,x)
        gvars[i] = None
        continue

    if n in remove_gvars:
        print "Removed  : %s" % n
        gvars[i] = None
        continue

    if n[0] not in '_P' or n[-1].lower() not in 'abcdefghijklmnopqrstuvwxyz':
        raise ValueError("not handled case: %s" % x)

    l.add(n)

gvars = sorted(x for x in gvars if x)
print "** %lu global variables found after cleanup" % len(gvars)

gvars_c_template = """/* GENERATED FILE. DO NOT EDIT IT MANUALLY */
#include \"libraries/python34_gvars.h\"
#include \"libheader.h\"

/* declared as weak symbols for final build (replaced if debug build) */

%s

__attribute__((section (".text.pyapi"))) void PyMorphOS_InitGVars(struct PyMorphOS_GVar_STRUCT *storage)
{
    PythonBase->PythonGVars = storage;

%s
}
"""

gvars_h_template = """/* GENERATED FILE. DO NOT EDIT IT MANUALLY */
#ifndef LIB_PYTHON_GVARS_H
#define LIB_PYTHON_GVARS_H

struct PyMorphOS_GVar_STRUCT {
%s
};

extern void PyMorphOS_InitGVars(struct PyMorphOS_GVar_STRUCT *);
extern struct PyMorphOS_GVar_STRUCT __pym_GVars;

#ifndef DONT_WRAP_VARS
%s
#endif /* !DONT_WRAP_VARS */
#endif /* LIB_PYTHON_GVARS_H */
"""

print "** Creating python34_gvars.c ..."
with open('python34_gvars.c', 'w') as f:
    f.write(gvars_c_template % ('\n'.join("extern int %s;" % x.split(';')[0] for x in gvars),
                                '\n'.join("    storage->p_%-35s = &%s;" % ((x.split(';')[0],)*2) for x in gvars)))

gvars_h_filename = "include/libraries/python34_gvars.h"
dirname = os.path.dirname(gvars_h_filename)
if not os.path.exists(dirname):
    os.mkdir(dirname)

print "** Creating '%s' ..." % gvars_h_filename
with open(gvars_h_filename, 'w') as f:
    f.write(gvars_h_template % ('\n'.join("    void* p_%s;" % x.split(';')[0] for x in gvars),
                                '\n'.join("#define %-35s (*(%s*)__pym_GVars.p_%s)" % tuple(x.split(';') + [ x.split(';')[0] ]) for x in gvars)))
