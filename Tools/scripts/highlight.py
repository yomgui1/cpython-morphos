#!/usr/bin/env python3
'''Add syntax highlighting to Python source code'''

__all__ = ['analyze_python', 'ansi_highlight', 'default_ansi',
           'html_highlight', 'build_html_page', 'default_css', 'default_html']

__author__ = 'Raymond Hettinger'

import keyword, tokenize, cgi, functools

def is_builtin(s):
    'Return True if s is the name of a builtin'
    return hasattr(__builtins__, s)

def combine_range(lines, start, end):
    'Join content from a range of lines between start and end'
    (srow, scol), (erow, ecol) = start, end
    if srow == erow:
        return lines[srow-1][scol:ecol], end
    rows = [lines[srow-1][scol:]] + lines[srow: erow-1] + [lines[erow-1][:ecol]]
    return ''.join(rows), end

def analyze_python(source):
    '''Generate and classify chunks of Python for syntax highlighting.
       Yields tuples in the form: (leadin_text, category, categorized_text).
       The final tuple has empty strings for the category and categorized text.

    '''
    lines = source.splitlines(True)
    lines.append('')
    readline = functools.partial(next, iter(lines), '')
    kind = tok_str = ''
    tok_type = tokenize.COMMENT
    written = (1, 0)
    for tok in tokenize.generate_tokens(readline):
        prev_tok_type, prev_tok_str = tok_type, tok_str
        tok_type, tok_str, (srow, scol), (erow, ecol), logical_lineno = tok
        kind = ''
        if tok_type == tokenize.COMMENT:
            kind = 'comment'
        elif tok_type == tokenize.OP and tok_str[:1] not in '{}[](),.:;':
            kind = 'operator'
        elif tok_type == tokenize.STRING:
            kind = 'string'
            if prev_tok_type == tokenize.INDENT or scol==0:
                kind = 'docstring'
        elif tok_type == tokenize.NAME:
            if tok_str in ('def', 'class', 'import', 'from'):
                kind = 'definition'
            elif prev_tok_str in ('def', 'class'):
                kind = 'defname'
            elif keyword.iskeyword(tok_str):
                kind = 'keyword'
            elif is_builtin(tok_str) and prev_tok_str != '.':
                kind = 'builtin'
        if kind:
            line_upto_token, written = combine_range(lines, written, (srow, scol))
            line_thru_token, written = combine_range(lines, written, (erow, ecol))
            yield line_upto_token, kind, line_thru_token
    line_upto_token, written = combine_range(lines, written, (erow, ecol))
    yield line_upto_token, '', ''

default_ansi = {
    'comment': ('\033[0;31m', '\033[0m'),
    'string': ('\033[0;32m', '\033[0m'),
    'docstring': ('\033[0;32m', '\033[0m'),
    'keyword': ('\033[0;33m', '\033[0m'),
    'builtin': ('\033[0;35m', '\033[0m'),
    'definition': ('\033[0;33m', '\033[0m'),
    'defname': ('\033[0;34m', '\033[0m'),
    'operator': ('\033[0;33m', '\033[0m'),
}

def ansi_highlight(classified_text, colors=default_ansi):
    'Add syntax highlighting to source code using ANSI escape sequences'
    # http://en.wikipedia.org/wiki/ANSI_escape_code
    result = []
    for line_upto_token, kind, line_thru_token in classified_text:
        opener, closer = colors.get(kind, ('', ''))
        result += [line_upto_token, opener, line_thru_token, closer]
    return ''.join(result)

def html_highlight(classified_text,opener='<pre class="python">\n', closer='</pre>\n'):
    'Convert classified text to an HTML fragment'
    result = [opener]
    for line_upto_token, kind, line_thru_token in classified_text:
        if kind:
            result += [cgi.escape(line_upto_token),
                       '<span class="%s">' % kind,
                       cgi.escape(line_thru_token),
                       '</span>']
        else:
            result += [cgi.escape(line_upto_token),
                       cgi.escape(line_thru_token)]
    result += [closer]
    return ''.join(result)

default_css = {
    '.comment': '{color: crimson;}',
    '.string':  '{color: forestgreen;}',
    '.docstring': '{color: forestgreen; font-style:italic;}',
    '.keyword': '{color: darkorange;}',
    '.builtin': '{color: purple;}',
    '.definition': '{color: darkorange; font-weight:bold;}',
    '.defname': '{color: blue;}',
    '.operator': '{color: brown;}',
}

default_html = '''\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
          "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta http-equiv="Content-type" content="text/html;charset=UTF-8">
<title> {title} </title>
<style type="text/css">
{css}
</style>
</head>
<body>
{body}
</body>
</html>
'''

def build_html_page(classified_text, title='python',
                    css=default_css, html=default_html):
    'Create a complete HTML page with colorized source code'
    css_str = '\n'.join(['%s %s' % item for item in css.items()])
    result = html_highlight(classified_text)
    title = cgi.escape(title)
    return html.format(title=title, css=css_str, body=result)


if __name__ == '__main__':
    import sys, argparse, webbrowser, os, textwrap

    parser = argparse.ArgumentParser(
            description = 'Add syntax highlighting to Python source code',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog = textwrap.dedent('''
                examples:

                  # Show syntax highlighted code in the terminal window
                  $ ./highlight.py myfile.py

                  # Colorize myfile.py and display in a browser
                  $ ./highlight.py -b myfile.py

                  # Create an HTML section to embed in an existing webpage
                  ./highlight.py -s myfile.py

                  # Create a complete HTML file
                  $ ./highlight.py -c myfile.py > myfile.html
            '''))
    parser.add_argument('sourcefile', metavar = 'SOURCEFILE',
            help = 'file containing Python sourcecode')
    parser.add_argument('-b', '--browser', action = 'store_true',
            help = 'launch a browser to show results')
    parser.add_argument('-c', '--complete', action = 'store_true',
            help = 'build a complete html webpage')
    parser.add_argument('-s', '--section', action = 'store_true',
            help = 'show an HTML section rather than a complete webpage')
    parser.add_argument('-v', '--verbose', action = 'store_true',
            help = 'display categorized text to stderr')
    args = parser.parse_args()

    if args.section and (args.browser or args.complete):
        parser.error('The -s/--section option is incompatible with '
                     'the -b/--browser or -c/--complete options')

    sourcefile = args.sourcefile
    with open(sourcefile) as f:
        source = f.read()
    classified_text = analyze_python(source)

    if args.verbose:
        classified_text = list(classified_text)
        for line_upto_token, kind, line_thru_token in classified_text:
            sys.stderr.write('%15s:  %r\n' % ('leadin', line_upto_token))
            sys.stderr.write('%15s:  %r\n\n' % (kind, line_thru_token))

    if args.complete or args.browser:
        encoded = build_html_page(classified_text, title=sourcefile)
    elif args.section:
        encoded = html_highlight(classified_text)
    else:
        encoded = ansi_highlight(classified_text)

    if args.browser:
        htmlfile = os.path.splitext(os.path.basename(sourcefile))[0] + '.html'
        with open(htmlfile, 'w') as f:
            f.write(encoded)
        webbrowser.open('file://' + os.path.abspath(htmlfile))
    else:
        sys.stdout.write(encoded)
