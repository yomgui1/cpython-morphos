# This script generates a Python interface for an Apple Macintosh Manager.
# It uses the "bgen" package to generate C code.
# The function specifications are generated by scanning the mamager's header file,
# using the "scantools" package (customized for this particular manager).

import string

# Declarations that change for each manager
MACHEADERFILE = 'Controls.h'		# The Apple header file
MODNAME = 'Ctl'				# The name of the module
OBJECTNAME = 'Control'			# The basic name of the objects used here

# The following is *usually* unchanged but may still require tuning
MODPREFIX = MODNAME			# The prefix for module-wide routines
OBJECTTYPE = OBJECTNAME + 'Handle'	# The C type used to represent them
OBJECTPREFIX = MODPREFIX + 'Obj'	# The prefix for object methods
INPUTFILE = string.lower(MODPREFIX) + 'gen.py' # The file generated by the scanner
OUTPUTFILE = MODNAME + "module.c"	# The file generated by this program

from macsupport import *

# Create the type objects

ControlHandle = OpaqueByValueType(OBJECTTYPE, OBJECTPREFIX)
ControlRef = ControlHandle
ExistingControlHandle = OpaqueByValueType(OBJECTTYPE, "CtlObj_WhichControl", "BUG")

RgnHandle = OpaqueByValueType("RgnHandle", "ResObj")
CCTabHandle = OpaqueByValueType("CCTabHandle", "ResObj")
AuxCtlHandle = OpaqueByValueType("AuxCtlHandle", "ResObj")
ControlPartCode = Type("ControlPartCode", "h")
DragConstraint = Type("DragConstraint", "h")
ControlVariant = Type("ControlVariant", "h")
IconTransformType = Type("IconTransformType", "h")
ControlButtonGraphicAlignment = Type("ControlButtonGraphicAlignment", "h")
ControlButtonTextAlignment = Type("ControlButtonTextAlignment", "h")
ControlButtonTextPlacement = Type("ControlButtonTextPlacement", "h")
ControlContentType = Type("ControlContentType", "h")
ControlFocusPart = Type("ControlFocusPart", "h")

ControlFontStyleRec = OpaqueType('ControlFontStyleRec', 'ControlFontStyle')
ControlFontStyleRec_ptr = ControlFontStyleRec

includestuff = includestuff + """
#include <%s>""" % MACHEADERFILE + """

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

extern PyObject *CtlObj_WhichControl(ControlHandle); /* Forward */
extern PyObject *QdRGB_New(RGBColorPtr);
extern QdRGB_Convert(PyObject *, RGBColorPtr);

#ifdef THINK_C
#define  ControlActionUPP ProcPtr
#endif

/*
** Parse/generate ControlFontStyleRec records
*/
#if 0 /* Not needed */
PyObject *ControlFontStyle_New(itself)
	ControlFontStyleRec *itself;
{

	return Py_BuildValue("hhhhhhO&O&", itself->flags, itself->font,
		itself->size, itself->style, itself->mode, itself->just,
		QdRGB_New, &itself->foreColor, QdRGB_New, &itself->backColor);
}
#endif

ControlFontStyle_Convert(v, itself)
	PyObject *v;
	ControlFontStyleRec *itself;
{
	return PyArg_ParseTuple(v, "hhhhhhO&O&", &itself->flags,
		&itself->font, &itself->size, &itself->style, &itself->mode, 
		&itself->just, QdRGB_Convert, &itself->foreColor, 
		QdRGB_Convert, &itself->backColor);
}

/* TrackControl callback support */
static PyObject *tracker;
static ControlActionUPP mytracker_upp;

extern int settrackfunc(PyObject *); 	/* forward */
extern void clrtrackfunc(void);	/* forward */
"""

finalstuff = finalstuff + """
PyObject *
CtlObj_WhichControl(ControlHandle c)
{
	PyObject *it;
	
	/* XXX What if we find a control belonging to some other package? */
	if (c == NULL)
		it = NULL;
	else
		it = (PyObject *) GetControlReference(c);
	if (it == NULL || ((ControlObject *)it)->ob_itself != c)
		it = Py_None;
	Py_INCREF(it);
	return it;
}

static int
settrackfunc(obj)
	PyObject *obj;
{
	if (tracker) {
		PyErr_SetString(Ctl_Error, "Tracker function in use");
		return 0;
	}
	tracker = obj;
	Py_INCREF(tracker);
}

static void
clrtrackfunc()
{
	Py_XDECREF(tracker);
	tracker = 0;
}

static pascal void
mytracker(ctl, part)
	ControlHandle ctl;
	short part;
{
	PyObject *args, *rv=0;
	
	args = Py_BuildValue("(O&i)", CtlObj_WhichControl, ctl, (int)part);
	if (args && tracker) {
		rv = PyEval_CallObject(tracker, args);
		Py_DECREF(args);
	}
	if (rv)
		Py_DECREF(rv);
	else
		fprintf(stderr, "TrackControl: exception in tracker function\\n");
}
"""

initstuff = initstuff + """
mytracker_upp = NewControlActionProc(mytracker);
"""

class MyObjectDefinition(GlobalObjectDefinition):
	def outputCheckNewArg(self):
		Output("if (itself == NULL) return PyMac_Error(resNotFound);")
	def outputInitStructMembers(self):
		GlobalObjectDefinition.outputInitStructMembers(self)
		Output("SetControlReference(itself, (long)it);")
	def outputCleanupStructMembers(self):
		Output("if (self->ob_itself) SetControlReference(self->ob_itself, (long)0); /* Make it forget about us */")
		
# Create the generator groups and link them
module = MacModule(MODNAME, MODPREFIX, includestuff, finalstuff, initstuff)
object = MyObjectDefinition(OBJECTNAME, OBJECTPREFIX, OBJECTTYPE)
module.addobject(object)

# Create the generator classes used to populate the lists
Function = OSErrFunctionGenerator
Method = OSErrMethodGenerator

# Create and populate the lists
functions = []
methods = []
execfile(INPUTFILE)
execfile('ctledit.py')

# add the populated lists to the generator groups
for f in functions: module.add(f)
for f in methods: object.add(f)

# Manual generator for TrackControl, due to callback ideosyncracies
trackcontrol_body = """
ControlPartCode _rv;
Point startPoint;
ControlActionUPP upp = 0;
PyObject *callback = 0;

if (!PyArg_ParseTuple(_args, "O&|O",
                      PyMac_GetPoint, &startPoint, &callback))
	return NULL;
if (callback && callback != Py_None) {
	if (PyInt_Check(callback) && PyInt_AS_LONG(callback) == -1)
		upp = (ControlActionUPP)-1;
	else {
		settrackfunc(callback);
		upp = mytracker_upp;
	}
}
_rv = TrackControl(_self->ob_itself,
                   startPoint,
                   upp);
clrtrackfunc();
_res = Py_BuildValue("h",
                     _rv);
return _res;
"""

f = ManualGenerator("TrackControl", trackcontrol_body);
#f.docstring = "(Point startPoint [,trackercallback]) -> (ControlPartCode _rv)"
object.add(f)

# And manual generators to get/set popup menu information
getpopupdata_body = """
PopupPrivateDataHandle hdl;

if ( (*_self->ob_itself)->contrlData == NULL ) {
	PyErr_SetString(Ctl_Error, "No contrlData handle in control");
	return 0;
}
hdl = (PopupPrivateDataHandle)(*_self->ob_itself)->contrlData;
HLock((Handle)hdl);
_res = Py_BuildValue("O&i", MenuObj_New, (*hdl)->mHandle, (int)(*hdl)->mID);
HUnlock((Handle)hdl);
return _res;
"""
f = ManualGenerator("GetPopupData", getpopupdata_body)
object.add(f)

setpopupdata_body = """
PopupPrivateDataHandle hdl;
MenuHandle mHandle;
short mID;

if (!PyArg_ParseTuple(_args, "O&h", MenuObj_Convert, &mHandle, &mID) )
	return 0;
if ( (*_self->ob_itself)->contrlData == NULL ) {
	PyErr_SetString(Ctl_Error, "No contrlData handle in control");
	return 0;
}
hdl = (PopupPrivateDataHandle)(*_self->ob_itself)->contrlData;
(*hdl)->mHandle = mHandle;
(*hdl)->mID = mID;
Py_INCREF(Py_None);
return Py_None;
"""
f = ManualGenerator("SetPopupData", setpopupdata_body)
object.add(f)


# generate output (open the output file as late as possible)
SetOutputFileName(OUTPUTFILE)
module.generate()
