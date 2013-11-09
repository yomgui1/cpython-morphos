import sys
import tkinter
import unittest

_tk_unavailable = None

def check_tk_availability():
    """Check that Tk is installed and available."""
    global _tk_unavailable

    if _tk_unavailable is None:
        _tk_unavailable = False
        if sys.platform == 'darwin':
            # The Aqua Tk implementations on OS X can abort the process if
            # being called in an environment where a window server connection
            # cannot be made, for instance when invoked by a buildbot or ssh
            # process not running under the same user id as the current console
            # user.  To avoid that, raise an exception if the window manager
            # connection is not available.
            from ctypes import cdll, c_int, pointer, Structure
            from ctypes.util import find_library

            app_services = cdll.LoadLibrary(find_library("ApplicationServices"))

            if app_services.CGMainDisplayID() == 0:
                _tk_unavailable = "cannot run without OS X window manager"
            else:
                class ProcessSerialNumber(Structure):
                    _fields_ = [("highLongOfPSN", c_int),
                                ("lowLongOfPSN", c_int)]
                psn = ProcessSerialNumber()
                psn_p = pointer(psn)
                if (  (app_services.GetCurrentProcess(psn_p) < 0) or
                      (app_services.SetFrontProcess(psn_p) < 0) ):
                    _tk_unavailable = "cannot run without OS X gui process"
        else:   # not OS X
            import tkinter
            try:
                tkinter.Button()
            except tkinter.TclError as msg:
                # assuming tk is not available
                _tk_unavailable = "tk not available: %s" % msg

    if _tk_unavailable:
        raise unittest.SkipTest(_tk_unavailable)
    return

def get_tk_root():
    check_tk_availability()     # raise exception if tk unavailable
    try:
        root = tkinter._default_root
    except AttributeError:
        # it is possible to disable default root in Tkinter, although
        # I haven't seen people doing it (but apparently someone did it
        # here).
        root = None

    if root is None:
        # create a new master only if there isn't one already
        root = tkinter.Tk()

    return root

def root_deiconify():
    root = get_tk_root()
    root.deiconify()

def root_withdraw():
    root = get_tk_root()
    root.withdraw()


def simulate_mouse_click(widget, x, y):
    """Generate proper events to click at the x, y position (tries to act
    like an X server)."""
    widget.event_generate('<Enter>', x=0, y=0)
    widget.event_generate('<Motion>', x=x, y=y)
    widget.event_generate('<ButtonPress-1>', x=x, y=y)
    widget.event_generate('<ButtonRelease-1>', x=x, y=y)


import _tkinter
tcl_version = tuple(map(int, _tkinter.TCL_VERSION.split('.')))

def requires_tcl(*version):
    return unittest.skipUnless(tcl_version >= version,
            'requires Tcl version >= ' + '.'.join(map(str, version)))

_tk_patchlevel = None
def get_tk_patchlevel():
    global _tk_patchlevel
    if _tk_patchlevel is None:
        tcl = tkinter.Tcl()
        patchlevel = []
        for x in tcl.call('info', 'patchlevel').split('.'):
            try:
                x = int(x, 10)
            except ValueError:
                x = -1
            patchlevel.append(x)
        _tk_patchlevel = tuple(patchlevel)
    return _tk_patchlevel

units = {
    'c': 72 / 2.54,     # centimeters
    'i': 72,            # inches
    'm': 72 / 25.4,     # millimeters
    'p': 1,             # points
}

def pixels_conv(value):
    return float(value[:-1]) * units[value[-1:]]

def tcl_obj_eq(actual, expected):
    if actual == expected:
        return True
    if isinstance(actual, _tkinter.Tcl_Obj):
        if isinstance(expected, str):
            return str(actual) == expected
    if isinstance(actual, tuple):
        if isinstance(expected, tuple):
            return (len(actual) == len(expected) and
                    all(tcl_obj_eq(act, exp)
                        for act, exp in zip(actual, expected)))
    return False

def widget_eq(actual, expected):
    if actual == expected:
        return True
    if isinstance(actual, (str, tkinter.Widget)):
        if isinstance(expected, (str, tkinter.Widget)):
            return str(actual) == str(expected)
    return False
