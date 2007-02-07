"""Unit tests for collections.defaultdict."""

import os
import copy
import tempfile
import unittest
from test import test_support

from collections import defaultdict

def foobar():
    return list

class TestDefaultDict(unittest.TestCase):

    def test_basic(self):
        d1 = defaultdict()
        self.assertEqual(d1.default_factory, None)
        d1.default_factory = list
        d1[12].append(42)
        self.assertEqual(d1, {12: [42]})
        d1[12].append(24)
        self.assertEqual(d1, {12: [42, 24]})
        d1[13]
        d1[14]
        self.assertEqual(d1, {12: [42, 24], 13: [], 14: []})
        self.assert_(d1[12] is not d1[13] is not d1[14])
        d2 = defaultdict(list, foo=1, bar=2)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, {"foo": 1, "bar": 2})
        self.assertEqual(d2["foo"], 1)
        self.assertEqual(d2["bar"], 2)
        self.assertEqual(d2[42], [])
        self.assert_("foo" in d2)
        self.assert_("foo" in d2.keys())
        self.assert_("bar" in d2)
        self.assert_("bar" in d2.keys())
        self.assert_(42 in d2)
        self.assert_(42 in d2.keys())
        self.assert_(12 not in d2)
        self.assert_(12 not in d2.keys())
        d2.default_factory = None
        self.assertEqual(d2.default_factory, None)
        try:
            d2[15]
        except KeyError, err:
            self.assertEqual(err.args, (15,))
        else:
            self.fail("d2[15] didn't raise KeyError")
        self.assertRaises(TypeError, defaultdict, 1)

    def test_missing(self):
        d1 = defaultdict()
        self.assertRaises(KeyError, d1.__missing__, 42)
        d1.default_factory = list
        self.assertEqual(d1.__missing__(42), [])

    def test_repr(self):
        d1 = defaultdict()
        self.assertEqual(d1.default_factory, None)
        self.assertEqual(repr(d1), "defaultdict(None, {})")
        d1[11] = 41
        self.assertEqual(repr(d1), "defaultdict(None, {11: 41})")
        d2 = defaultdict(int)
        self.assertEqual(d2.default_factory, int)
        d2[12] = 42
        self.assertEqual(repr(d2), "defaultdict(<type 'int'>, {12: 42})")
        def foo(): return 43
        d3 = defaultdict(foo)
        self.assert_(d3.default_factory is foo)
        d3[13]
        self.assertEqual(repr(d3), "defaultdict(%s, {13: 43})" % repr(foo))

    def test_print(self):
        d1 = defaultdict()
        def foo(): return 42
        d2 = defaultdict(foo, {1: 2})
        # NOTE: We can't use tempfile.[Named]TemporaryFile since this
        # code must exercise the tp_print C code, which only gets
        # invoked for *real* files.
        tfn = tempfile.mktemp()
        try:
            f = open(tfn, "w+")
            try:
                print >>f, d1
                print >>f, d2
                f.seek(0)
                self.assertEqual(f.readline(), repr(d1) + "\n")
                self.assertEqual(f.readline(), repr(d2) + "\n")
            finally:
                f.close()
        finally:
            os.remove(tfn)

    def test_copy(self):
        d1 = defaultdict()
        d2 = d1.copy()
        self.assertEqual(type(d2), defaultdict)
        self.assertEqual(d2.default_factory, None)
        self.assertEqual(d2, {})
        d1.default_factory = list
        d3 = d1.copy()
        self.assertEqual(type(d3), defaultdict)
        self.assertEqual(d3.default_factory, list)
        self.assertEqual(d3, {})
        d1[42]
        d4 = d1.copy()
        self.assertEqual(type(d4), defaultdict)
        self.assertEqual(d4.default_factory, list)
        self.assertEqual(d4, {42: []})
        d4[12]
        self.assertEqual(d4, {42: [], 12: []})

    def test_shallow_copy(self):
        d1 = defaultdict(foobar, {1: 1})
        d2 = copy.copy(d1)
        self.assertEqual(d2.default_factory, foobar)
        self.assertEqual(d2, d1)
        d1.default_factory = list
        d2 = copy.copy(d1)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, d1)

    def test_deep_copy(self):
        d1 = defaultdict(foobar, {1: [1]})
        d2 = copy.deepcopy(d1)
        self.assertEqual(d2.default_factory, foobar)
        self.assertEqual(d2, d1)
        self.assert_(d1[1] is not d2[1])
        d1.default_factory = list
        d2 = copy.deepcopy(d1)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, d1)


def test_main():
    test_support.run_unittest(TestDefaultDict)

if __name__ == "__main__":
    test_main()
