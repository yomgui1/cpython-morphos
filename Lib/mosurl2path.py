"""Morphos-specific module for conversion between pathnames and URLs.

Do not import directly; use urllib instead."""

import urllib
import os

__all__ = ["url2pathname","pathname2url"]

def url2pathname(pathname):
    "Convert /-delimited pathname to Morphos pathname"
    #
    # BUG: fix .. handling
    #
    tp = urllib.splittype(pathname)[0]
    if tp and tp != 'file':
        raise RuntimeError, 'Cannot convert non-local URL to pathname'
    # Turn starting /// into /, an empty hostname means current host
    if pathname[:3] == '///':
        pathname = pathname[2:]
    elif pathname[:2] == '//':
        raise RuntimeError, 'Cannot convert non-local URL to pathname'
    components = pathname.split('/')
    # Remove . and convert embedded .. into //
    i = 0
    while i < len(components):
        if components[i] == '.':
            del components[i]
        elif components[i] == '..' and i > 0 and components[i-1] != '..':
            del components[i-1:i+1]
            i = i-1
        elif components[i] == '' and i > 0 and components[i-1] != '':
            del components[i]
        else:
            i = i+1
    i = 0
    while i < len(components):
        if components[i] == '..':
            components[i] = ''
        i = i+1
    if not components[0]:
        if i > 2:
            del components[0]    
            rv = ':'
        else:
            rv = ''
    elif i > 1:
        rv = '/'
    else:
        rv = ''
    rv = components[0] + rv + '/'.join(components[1:])
    # and finally unquote slashes and other funny characters
    return urllib.unquote(rv)

def pathname2url(pathname):
    "convert morphos pathname to /-delimited pathname"
    components = pathname.split(':')
    # Remove empty first and/or last component
    if components[0] == '':
        del components[0]
    if components[-1] == '':
        del components[-1]
    # Replace empty string ('//') by .. (will result in '/../' later)
    for i in range(len(components)):
        if components[i] == '':
            components[i] = '..'
    # Truncate names longer than 31 bytes
    components = map(_pncomp2url, components)

    if os.path.isabs(pathname):
        return '/' + '/'.join(components)
    else:
        return '/'.join(components)

def _pncomp2url(component):
    component = urllib.quote(component[:31], safe='')  # We want to quote slashes
    return component

def test():
    for url in ["index.html",
                "bar/index.html",
                "/foo/bar/index.html",
                "/foo/bar/",
                "/",
                "/foo/./bar/index.html",
                "/foo/./bar/../index.html",
                "/foo/bar/../../index.html",
                "/foo/../../index.html",
                "./index.html",
                "../index.html",
                "../../index.html"]:
        print '%r -> %r' % (url, url2pathname(url))
    for path in ["drive:",
                 "drive:dir/",
                 "drive:dir/file",
                 "drive:file",
                 "file",
                 ":file",
                 ":dir/file"]:
        print '%r -> %r' % (path, pathname2url(path))

if __name__ == '__main__':
    test()
