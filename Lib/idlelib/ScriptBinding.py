"""Extension to execute code outside the Python shell window.

This adds the following commands:

- Check module does a full syntax check of the current module.
  It also runs the tabnanny to catch any inconsistent tabs.

- Run module executes the module's code in the __main__ namespace.  The window
  must have been saved previously. The module is added to sys.modules, and is
  also added to the __main__ namespace.

XXX GvR Redesign this interface (yet again) as follows:

- Present a dialog box for ``Run script''

- Allow specify command line arguments in the dialog box

"""

import re
import string
import tabnanny
import tokenize
import tkMessageBox

IDENTCHARS = string.ascii_letters + string.digits + "_"

indent_message = """Error: Inconsistent indentation detected!

This means that either:

1) your indentation is outright incorrect (easy to fix), or

2) your indentation mixes tabs and spaces in a way that depends on \
how many spaces a tab is worth.

To fix case 2, change all tabs to spaces by using Select All followed \
by Untabify Region (both in the Edit menu)."""


# XXX 11Jun02 KBK TBD Implement stop-execution

class ScriptBinding:

    menudefs = [
        ('run', [None,
                 ('Check Module', '<<check-module>>'),
                 ('Run Script', '<<run-script>>'), ]), ]

    def __init__(self, editwin):
        self.editwin = editwin
        # Provide instance variables referenced by Debugger
        # XXX This should be done differently
        self.flist = self.editwin.flist
        self.root = self.flist.root

    def check_module_event(self, event):
        filename = self.getfilename()
        if not filename:
            return
        if not self.tabnanny(filename):
            return
        self.checksyntax(filename)

    def tabnanny(self, filename):
        f = open(filename, 'r')
        try:
            tabnanny.process_tokens(tokenize.generate_tokens(f.readline))
        except tokenize.TokenError, msg:
            self.errorbox("Token error", "Token error:\n%s" % msg)
            return False
        except tabnanny.NannyNag, nag:
            # The error messages from tabnanny are too confusing...
            self.editwin.gotoline(nag.get_lineno())
            self.errorbox("Tab/space error", indent_message)
            return False
        return True

    def checksyntax(self, filename):
        f = open(filename, 'r')
        source = f.read()
        f.close()
        if '\r' in source:
            source = re.sub(r"\r\n", "\n", source)
        if source and source[-1] != '\n':
            source = source + '\n'
        try:
            # If successful, return the compiled code
            return compile(source, filename, "exec")
        except (SyntaxError, OverflowError), err:
            try:
                msg, (errorfilename, lineno, offset, line) = err
                if not errorfilename:
                    err.args = msg, (filename, lineno, offset, line)
                    err.filename = filename
                self.colorize_syntax_error(msg, lineno, offset)
            except:
                msg = "*** " + str(err)
            self.errorbox("Syntax error",
                          "There's an error in your program:\n" + msg)
            return False
        
    def colorize_syntax_error(self, msg, lineno, offset):
        text = self.editwin.text
        pos = "0.0 + %d lines + %d chars" % (lineno-1, offset-1)
        text.tag_add("ERROR", pos)
        char = text.get(pos)
        if char and char in IDENTCHARS:
            text.tag_add("ERROR", pos + " wordstart", pos)
        if '\n' == text.get(pos):   # error at line end
            text.mark_set("insert", pos)
        else:
            text.mark_set("insert", pos + "+1c")
        text.see(pos)
        
    def run_script_event(self, event):
        "Check syntax, if ok run the script in the shell top level"
        filename = self.getfilename()
        if not filename:
            return
        code = self.checksyntax(filename)
        if not code:
            return
        flist = self.editwin.flist
        shell = flist.open_shell()
        interp = shell.interp
        if interp.tkconsole.executing:
            interp.display_executing_dialog()
            return
        interp.restart_subprocess()
        # XXX Too often this discards arguments the user just set...
        interp.runcommand("""if 1:
            _filename = %s
            import sys as _sys
            from os.path import basename as _basename
            if (not _sys.argv or
                _basename(_sys.argv[0]) != _basename(_filename)):
                _sys.argv = [_filename]
                del _filename, _sys, _basename
                \n""" % `filename`)
        interp.runcode(code)

    def getfilename(self):
        # Logic to make sure we have a saved filename
        # XXX Better logic would offer to save!
        if not self.editwin.get_saved():
            name = (self.editwin.short_title() or
                    self.editwin.long_title() or
                    "Untitled")
            self.errorbox("Not saved",
                          "The buffer for %s is not saved.\n" % name +
                          "Please save it first!")
            self.editwin.text.focus_set()
            return
        filename = self.editwin.io.filename
        if not filename:
            self.errorbox("No file name",
                          "This window has no file name")
            return
        return filename

    def errorbox(self, title, message):
        # XXX This should really be a function of EditorWindow...
        tkMessageBox.showerror(title, message, master=self.editwin.text)
        self.editwin.text.focus_set()
