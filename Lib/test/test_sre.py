# FIXME: this is basically test_re.py, with a few minor changes

import sys
sys.path=['.']+sys.path

from test_support import verbose, TestFailed
import sre
import sys, os, string, traceback

# Misc tests from Tim Peters' re.doc

if verbose:
    print 'Running tests on sre.search and sre.match'

try:
    assert sre.search('x*', 'axx').span(0) == (0, 0)
    assert sre.search('x*', 'axx').span() == (0, 0)
    assert sre.search('x+', 'axx').span(0) == (1, 3)
    assert sre.search('x+', 'axx').span() == (1, 3)
    assert sre.search('x', 'aaa') == None
except:
    raise TestFailed, "sre.search"

try:
    assert sre.match('a*', 'xxx').span(0) == (0, 0)
    assert sre.match('a*', 'xxx').span() == (0, 0)
    assert sre.match('x*', 'xxxa').span(0) == (0, 3)
    assert sre.match('x*', 'xxxa').span() == (0, 3)
    assert sre.match('a+', 'xxx') == None
except:
    raise TestFailed, "sre.search"

if verbose:
    print 'Running tests on sre.sub'

try:
    assert sre.sub("(?i)b+", "x", "bbbb BBBB") == 'x x'

    def bump_num(matchobj):
        int_value = int(matchobj.group(0))
        return str(int_value + 1)

    assert sre.sub(r'\d+', bump_num, '08.2 -2 23x99y') == '9.3 -3 24x100y'
    assert sre.sub(r'\d+', bump_num, '08.2 -2 23x99y', 3) == '9.3 -3 23x99y'

    assert sre.sub('.', lambda m: r"\n", 'x') == '\\n'
    assert sre.sub('.', r"\n", 'x') == '\n'

    s = r"\1\1"
    assert sre.sub('(.)', s, 'x') == 'xx'
    assert sre.sub('(.)', sre.escape(s), 'x') == s
    assert sre.sub('(.)', lambda m: s, 'x') == s

    assert sre.sub('(?P<a>x)', '\g<a>\g<a>', 'xx') == 'xxxx'
    assert sre.sub('(?P<a>x)', '\g<a>\g<1>', 'xx') == 'xxxx'
    assert sre.sub('(?P<unk>x)', '\g<unk>\g<unk>', 'xx') == 'xxxx'
    assert sre.sub('(?P<unk>x)', '\g<1>\g<1>', 'xx') == 'xxxx'

    assert sre.sub('a', r'\t\n\v\r\f\a\b\B\Z\a\A\w\W\s\S\d\D', 'a') == '\t\n\v\r\f\a\b\\B\\Z\a\\A\\w\\W\\s\\S\\d\\D'
    assert sre.sub('a', '\t\n\v\r\f\a', 'a') == '\t\n\v\r\f\a'
    assert sre.sub('a', '\t\n\v\r\f\a', 'a') == (chr(9)+chr(10)+chr(11)+chr(13)+chr(12)+chr(7))

    assert sre.sub('^\s*', 'X', 'test') == 'Xtest'
except AssertionError:
    raise TestFailed, "sre.sub"


try:
    assert sre.sub('a', 'b', 'aaaaa') == 'bbbbb'
    assert sre.sub('a', 'b', 'aaaaa', 1) == 'baaaa'
except AssertionError:
    raise TestFailed, "qualified sre.sub"

if verbose:
    print 'Running tests on symbolic references'

try:
    sre.sub('(?P<a>x)', '\g<a', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)', '\g<', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)', '\g', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)', '\g<a a>', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)', '\g<1a1>', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)', '\g<ab>', 'xx')
except IndexError, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)|(?P<b>y)', '\g<b>', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    sre.sub('(?P<a>x)|(?P<b>y)', '\\2', 'xx')
except sre.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

if verbose:
    print 'Running tests on sre.subn'

try:
    assert sre.subn("(?i)b+", "x", "bbbb BBBB") == ('x x', 2)
    assert sre.subn("b+", "x", "bbbb BBBB") == ('x BBBB', 1)
    assert sre.subn("b+", "x", "xyz") == ('xyz', 0)
    assert sre.subn("b*", "x", "xyz") == ('xxxyxzx', 4)
    assert sre.subn("b*", "x", "xyz", 2) == ('xxxyz', 2)
except AssertionError:
    raise TestFailed, "sre.subn"

if verbose:
    print 'Running tests on sre.split'

try:
    assert sre.split(":", ":a:b::c") == ['', 'a', 'b', '', 'c']
    assert sre.split(":*", ":a:b::c") == ['', 'a', 'b', 'c']
    assert sre.split("(:*)", ":a:b::c") == ['', ':', 'a', ':', 'b', '::', 'c']
    assert sre.split("(?::*)", ":a:b::c") == ['', 'a', 'b', 'c']
    assert sre.split("(:)*", ":a:b::c") == ['', ':', 'a', ':', 'b', ':', 'c']
    assert sre.split("([b:]+)", ":a:b::c") == ['', ':', 'a', ':b::', 'c']
    assert sre.split("(b)|(:+)", ":a:b::c") == \
           ['', None, ':', 'a', None, ':', '', 'b', None, '', None, '::', 'c']
    assert sre.split("(?:b)|(?::+)", ":a:b::c") == ['', 'a', '', '', 'c']
except AssertionError:
    raise TestFailed, "sre.split"

try:
    assert sre.split(":", ":a:b::c", 2) == ['', 'a', 'b::c']
    assert sre.split(':', 'a:b:c:d', 2) == ['a', 'b', 'c:d']

    assert sre.split("(:)", ":a:b::c", 2) == ['', ':', 'a', ':', 'b::c']
    assert sre.split("(:*)", ":a:b::c", 2) == ['', ':', 'a', ':', 'b::c']
except AssertionError:
    raise TestFailed, "qualified sre.split"

if verbose:
    print "Running tests on sre.findall"

try:
    assert sre.findall(":+", "abc") == []
    assert sre.findall(":+", "a:b::c:::d") == [":", "::", ":::"]
    assert sre.findall("(:+)", "a:b::c:::d") == [":", "::", ":::"]
    assert sre.findall("(:)(:*)", "a:b::c:::d") == [(":", ""),
                                                   (":", ":"),
                                                   (":", "::")]
except AssertionError:
    raise TestFailed, "sre.findall"

if verbose:
    print "Running tests on sre.match"

try:
    # No groups at all
    m = sre.match('a', 'a') ; assert m.groups() == ()
    # A single group
    m = sre.match('(a)', 'a') ; assert m.groups() == ('a',)

    pat = sre.compile('((a)|(b))(c)?')
    assert pat.match('a').groups() == ('a', 'a', None, None)
    assert pat.match('b').groups() == ('b', None, 'b', None)
    assert pat.match('ac').groups() == ('a', 'a', None, 'c')
    assert pat.match('bc').groups() == ('b', None, 'b', 'c')
    assert pat.match('bc').groups("") == ('b', "", 'b', 'c')
except AssertionError:
    raise TestFailed, "match .groups() method"

try:
    # A single group
    m = sre.match('(a)', 'a')
    assert m.group(0) == 'a' ; assert m.group(0) == 'a'
    assert m.group(1) == 'a' ; assert m.group(1, 1) == ('a', 'a')

    pat = sre.compile('(?:(?P<a1>a)|(?P<b2>b))(?P<c3>c)?')
    assert pat.match('a').group(1, 2, 3) == ('a', None, None)
    assert pat.match('b').group('a1', 'b2', 'c3') == (None, 'b', None)
    assert pat.match('ac').group(1, 'b2', 3) == ('a', None, 'c')
except AssertionError:
    raise TestFailed, "match .group() method"

if verbose:
    print "Running tests on sre.escape"

try:
    p=""
    for i in range(0, 256):
        p = p + chr(i)
        assert sre.match(sre.escape(chr(i)), chr(i)) != None
        assert sre.match(sre.escape(chr(i)), chr(i)).span() == (0,1)

    pat=sre.compile( sre.escape(p) )
    assert pat.match(p) != None
    assert pat.match(p).span() == (0,256)
except AssertionError:
    raise TestFailed, "sre.escape"


if verbose:
    print 'Pickling a SRE_Pattern instance'

try:
    import pickle
    pat = sre.compile('a(?:b|(c|e){1,2}?|d)+?(.)')
    s = pickle.dumps(pat)
    pat = pickle.loads(s)
except:
    print TestFailed, 're module pickle' # expected

try:
    import cPickle
    pat = sre.compile('a(?:b|(c|e){1,2}?|d)+?(.)')
    s = cPickle.dumps(pat)
    pat = cPickle.loads(s)
except:
    print TestFailed, 're module cPickle' # expected

try:
    assert sre.I == sre.IGNORECASE
    assert sre.L == sre.LOCALE
    assert sre.M == sre.MULTILINE
    assert sre.S == sre.DOTALL
    assert sre.X == sre.VERBOSE
    assert sre.T == sre.TEMPLATE
    assert sre.U == sre.UNICODE
except AssertionError:
    raise TestFailed, 're module constants'

for flags in [sre.I, sre.M, sre.X, sre.S, sre.L, sre.T, sre.U]:
    try:
        r = sre.compile('^pattern$', flags)
    except:
        print 'Exception raised on flag', flags

from re_tests import *

if verbose:
    print 'Running re_tests test suite'
else:
    # To save time, only run the first and last 10 tests
    #tests = tests[:10] + tests[-10:]
    pass

for t in tests:
    sys.stdout.flush()
    pattern=s=outcome=repl=expected=None
    if len(t)==5:
        pattern, s, outcome, repl, expected = t
    elif len(t)==3:
        pattern, s, outcome = t
    else:
        raise ValueError, ('Test tuples should have 3 or 5 fields',t)

    try:
        obj=sre.compile(pattern)
    except sre.error:
        if outcome==SYNTAX_ERROR: pass  # Expected a syntax error
        else:
            print '=== Syntax error:', t
    except KeyboardInterrupt: raise KeyboardInterrupt
    except:
        print '*** Unexpected error ***', t
        if verbose:
            traceback.print_exc(file=sys.stdout)
    else:
        try:
            result=obj.search(s)
        except (sre.error), msg:
            print '=== Unexpected exception', t, repr(msg)
        if outcome==SYNTAX_ERROR:
            # This should have been a syntax error; forget it.
            pass
        elif outcome==FAIL:
            if result is None: pass   # No match, as expected
            else: print '=== Succeeded incorrectly', t
        elif outcome==SUCCEED:
            if result is not None:
                # Matched, as expected, so now we compute the
                # result string and compare it to our expected result.
                start, end = result.span(0)
                vardict={'found': result.group(0),
                         'groups': result.group(),
                         'flags': result.re.flags}
                for i in range(1, 100):
                    try:
                        gi = result.group(i)
                        # Special hack because else the string concat fails:
                        if gi is None:
                            gi = "None"
                    except IndexError:
                        gi = "Error"
                    vardict['g%d' % i] = gi
                for i in result.re.groupindex.keys():
                    try:
                        gi = result.group(i)
                        if gi is None:
                            gi = "None"
                    except IndexError:
                        gi = "Error"
                    vardict[i] = gi
                repl=eval(repl, vardict)
                if repl!=expected:
                    print '=== grouping error', t,
                    print repr(repl)+' should be '+repr(expected)
            else:
                print '=== Failed incorrectly', t
                continue

            # Try the match on a unicode string, and check that it
            # still succeeds.
            result=obj.search(unicode(s, "latin-1"))
            if result==None:
                print '=== Fails on unicode match', t

            # Try the match on a unicode pattern, and check that it
            # still succeeds.
            obj=sre.compile(unicode(pattern, "latin-1"))
            result=obj.search(s)
            if result==None:
                print '=== Fails on unicode pattern match', t

            # Try the match with the search area limited to the extent
            # of the match and see if it still succeeds.  \B will
            # break (because it won't match at the end or start of a
            # string), so we'll ignore patterns that feature it.

            if pattern[:2]!='\\B' and pattern[-2:]!='\\B':
                obj=sre.compile(pattern)
                result=obj.search(s, result.start(0), result.end(0)+1)
                if result==None:
                    print '=== Failed on range-limited match', t

            # Try the match with IGNORECASE enabled, and check that it
            # still succeeds.
            obj=sre.compile(pattern, sre.IGNORECASE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on case-insensitive match', t

            # Try the match with LOCALE enabled, and check that it
            # still succeeds.
            obj=sre.compile(pattern, sre.LOCALE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on locale-sensitive match', t

            # Try the match with UNICODE locale enabled, and check
            # that it still succeeds.
            obj=sre.compile(pattern, sre.UNICODE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on unicode-sensitive match', t
