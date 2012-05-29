/* GENERATED FILE. DO NOT EDIT IT MANUALLY */
#include "libraries/python2_gvars.h"
#include "libheader.h"

/* declared as weak symbols for final build (replaced if debug build) */

extern int PyBaseObject_Type;
extern int PyBaseString_Type;
extern int PyBool_Type;
extern int PyBuffer_Type;
extern int PyCFunction_Type;
extern int PyCObject_Type;
extern int PyCallIter_Type;
extern int PyCell_Type;
extern int PyClassMethod_Type;
extern int PyClass_Type;
extern int PyCode_Type;
extern int PyComplex_Type;
extern int PyDict_Type;
extern int PyEnum_Type;
extern int PyExc_ArithmeticError;
extern int PyExc_AssertionError;
extern int PyExc_AttributeError;
extern int PyExc_BaseException;
extern int PyExc_DeprecationWarning;
extern int PyExc_EOFError;
extern int PyExc_EnvironmentError;
extern int PyExc_Exception;
extern int PyExc_FloatingPointError;
extern int PyExc_FutureWarning;
extern int PyExc_GeneratorExit;
extern int PyExc_IOError;
extern int PyExc_ImportError;
extern int PyExc_ImportWarning;
extern int PyExc_IndentationError;
extern int PyExc_IndexError;
extern int PyExc_KeyError;
extern int PyExc_KeyboardInterrupt;
extern int PyExc_LookupError;
extern int PyExc_MemoryError;
extern int PyExc_MemoryErrorInst;
extern int PyExc_NameError;
extern int PyExc_NotImplementedError;
extern int PyExc_OSError;
extern int PyExc_OverflowError;
extern int PyExc_PendingDeprecationWarning;
extern int PyExc_ReferenceError;
extern int PyExc_RuntimeError;
extern int PyExc_RuntimeWarning;
extern int PyExc_StandardError;
extern int PyExc_StopIteration;
extern int PyExc_SyntaxError;
extern int PyExc_SyntaxWarning;
extern int PyExc_SystemError;
extern int PyExc_SystemExit;
extern int PyExc_TabError;
extern int PyExc_TypeError;
extern int PyExc_UnboundLocalError;
extern int PyExc_UnicodeDecodeError;
extern int PyExc_UnicodeEncodeError;
extern int PyExc_UnicodeError;
extern int PyExc_UnicodeTranslateError;
extern int PyExc_UnicodeWarning;
extern int PyExc_UserWarning;
extern int PyExc_ValueError;
extern int PyExc_Warning;
extern int PyExc_ZeroDivisionError;
extern int PyFile_Type;
extern int PyFloat_Type;
extern int PyFrame_Type;
extern int PyFrozenSet_Type;
extern int PyFunction_Type;
extern int PyGen_Type;
extern int PyImport_FrozenModules;
extern int PyImport_Inittab;
extern int PyInstance_Type;
extern int PyInt_Type;
extern int PyList_Type;
extern int PyLong_Type;
extern int PyMethod_Type;
extern int PyModule_Type;
extern int PyOS_InputHook;
extern int PyOS_ReadlineFunctionPointer;
extern int PyProperty_Type;
extern int PyRange_Type;
extern int PyReversed_Type;
extern int PySTEntry_Type;
extern int PySeqIter_Type;
extern int PySet_Type;
extern int PySlice_Type;
extern int PyStaticMethod_Type;
extern int PyString_Type;
extern int PySuper_Type;
extern int PyTraceBack_Type;
extern int PyTuple_Type;
extern int PyType_Type;
extern int PyUnicode_Type;
extern int PyWrapperDescr_Type;
extern int Py_DebugFlag;
extern int Py_DivisionWarningFlag;
extern int Py_FileSystemDefaultEncoding;
extern int Py_FrozenFlag;
extern int Py_IgnoreEnvironmentFlag;
extern int Py_InteractiveFlag;
extern int Py_NoSiteFlag;
extern int Py_OptimizeFlag;
extern int Py_TabcheckFlag;
extern int Py_UnicodeFlag;
extern int Py_UseClassExceptionsFlag;
extern int Py_VerboseFlag;
extern int _PyLong_DigitValue;
extern int _PyOS_ReadlineTState;
extern int _PyOS_optarg;
extern int _PyOS_opterr;
extern int _PyOS_optind;
extern int _PyParser_TokenNames;
extern int _PyThreadState_Current;
extern int _PyThreadState_GetFrame;
extern int _PyTrash_delete_later;
extern int _PyTrash_delete_nesting;
extern int _PyWeakref_CallableProxyType;
extern int _PyWeakref_ProxyType;
extern int _PyWeakref_RefType;
extern int _Py_CheckInterval;
extern int _Py_CheckRecursionLimit;
extern int _Py_EllipsisObject;
extern int _Py_NoneStruct;
extern int _Py_NotImplementedStruct;
extern int _Py_PackageContext;
extern int _Py_QnewFlag;
extern int _Py_SwappedOp;
extern int _Py_Ticker;
extern int _Py_TrueStruct;
extern int _Py_ZeroStruct;

void PyMorphOS_InitGVars(struct PyMorphOS_GVar_STRUCT *storage)
{
    PythonBase->PythonGVars = storage;

    storage->p_PyBaseObject_Type                   = &PyBaseObject_Type;
    storage->p_PyBaseString_Type                   = &PyBaseString_Type;
    storage->p_PyBool_Type                         = &PyBool_Type;
    storage->p_PyBuffer_Type                       = &PyBuffer_Type;
    storage->p_PyCFunction_Type                    = &PyCFunction_Type;
    storage->p_PyCObject_Type                      = &PyCObject_Type;
    storage->p_PyCallIter_Type                     = &PyCallIter_Type;
    storage->p_PyCell_Type                         = &PyCell_Type;
    storage->p_PyClassMethod_Type                  = &PyClassMethod_Type;
    storage->p_PyClass_Type                        = &PyClass_Type;
    storage->p_PyCode_Type                         = &PyCode_Type;
    storage->p_PyComplex_Type                      = &PyComplex_Type;
    storage->p_PyDict_Type                         = &PyDict_Type;
    storage->p_PyEnum_Type                         = &PyEnum_Type;
    storage->p_PyExc_ArithmeticError               = &PyExc_ArithmeticError;
    storage->p_PyExc_AssertionError                = &PyExc_AssertionError;
    storage->p_PyExc_AttributeError                = &PyExc_AttributeError;
    storage->p_PyExc_BaseException                 = &PyExc_BaseException;
    storage->p_PyExc_DeprecationWarning            = &PyExc_DeprecationWarning;
    storage->p_PyExc_EOFError                      = &PyExc_EOFError;
    storage->p_PyExc_EnvironmentError              = &PyExc_EnvironmentError;
    storage->p_PyExc_Exception                     = &PyExc_Exception;
    storage->p_PyExc_FloatingPointError            = &PyExc_FloatingPointError;
    storage->p_PyExc_FutureWarning                 = &PyExc_FutureWarning;
    storage->p_PyExc_GeneratorExit                 = &PyExc_GeneratorExit;
    storage->p_PyExc_IOError                       = &PyExc_IOError;
    storage->p_PyExc_ImportError                   = &PyExc_ImportError;
    storage->p_PyExc_ImportWarning                 = &PyExc_ImportWarning;
    storage->p_PyExc_IndentationError              = &PyExc_IndentationError;
    storage->p_PyExc_IndexError                    = &PyExc_IndexError;
    storage->p_PyExc_KeyError                      = &PyExc_KeyError;
    storage->p_PyExc_KeyboardInterrupt             = &PyExc_KeyboardInterrupt;
    storage->p_PyExc_LookupError                   = &PyExc_LookupError;
    storage->p_PyExc_MemoryError                   = &PyExc_MemoryError;
    storage->p_PyExc_MemoryErrorInst               = &PyExc_MemoryErrorInst;
    storage->p_PyExc_NameError                     = &PyExc_NameError;
    storage->p_PyExc_NotImplementedError           = &PyExc_NotImplementedError;
    storage->p_PyExc_OSError                       = &PyExc_OSError;
    storage->p_PyExc_OverflowError                 = &PyExc_OverflowError;
    storage->p_PyExc_PendingDeprecationWarning     = &PyExc_PendingDeprecationWarning;
    storage->p_PyExc_ReferenceError                = &PyExc_ReferenceError;
    storage->p_PyExc_RuntimeError                  = &PyExc_RuntimeError;
    storage->p_PyExc_RuntimeWarning                = &PyExc_RuntimeWarning;
    storage->p_PyExc_StandardError                 = &PyExc_StandardError;
    storage->p_PyExc_StopIteration                 = &PyExc_StopIteration;
    storage->p_PyExc_SyntaxError                   = &PyExc_SyntaxError;
    storage->p_PyExc_SyntaxWarning                 = &PyExc_SyntaxWarning;
    storage->p_PyExc_SystemError                   = &PyExc_SystemError;
    storage->p_PyExc_SystemExit                    = &PyExc_SystemExit;
    storage->p_PyExc_TabError                      = &PyExc_TabError;
    storage->p_PyExc_TypeError                     = &PyExc_TypeError;
    storage->p_PyExc_UnboundLocalError             = &PyExc_UnboundLocalError;
    storage->p_PyExc_UnicodeDecodeError            = &PyExc_UnicodeDecodeError;
    storage->p_PyExc_UnicodeEncodeError            = &PyExc_UnicodeEncodeError;
    storage->p_PyExc_UnicodeError                  = &PyExc_UnicodeError;
    storage->p_PyExc_UnicodeTranslateError         = &PyExc_UnicodeTranslateError;
    storage->p_PyExc_UnicodeWarning                = &PyExc_UnicodeWarning;
    storage->p_PyExc_UserWarning                   = &PyExc_UserWarning;
    storage->p_PyExc_ValueError                    = &PyExc_ValueError;
    storage->p_PyExc_Warning                       = &PyExc_Warning;
    storage->p_PyExc_ZeroDivisionError             = &PyExc_ZeroDivisionError;
    storage->p_PyFile_Type                         = &PyFile_Type;
    storage->p_PyFloat_Type                        = &PyFloat_Type;
    storage->p_PyFrame_Type                        = &PyFrame_Type;
    storage->p_PyFrozenSet_Type                    = &PyFrozenSet_Type;
    storage->p_PyFunction_Type                     = &PyFunction_Type;
    storage->p_PyGen_Type                          = &PyGen_Type;
    storage->p_PyImport_FrozenModules              = &PyImport_FrozenModules;
    storage->p_PyImport_Inittab                    = &PyImport_Inittab;
    storage->p_PyInstance_Type                     = &PyInstance_Type;
    storage->p_PyInt_Type                          = &PyInt_Type;
    storage->p_PyList_Type                         = &PyList_Type;
    storage->p_PyLong_Type                         = &PyLong_Type;
    storage->p_PyMethod_Type                       = &PyMethod_Type;
    storage->p_PyModule_Type                       = &PyModule_Type;
    storage->p_PyOS_InputHook                      = &PyOS_InputHook;
    storage->p_PyOS_ReadlineFunctionPointer        = &PyOS_ReadlineFunctionPointer;
    storage->p_PyProperty_Type                     = &PyProperty_Type;
    storage->p_PyRange_Type                        = &PyRange_Type;
    storage->p_PyReversed_Type                     = &PyReversed_Type;
    storage->p_PySTEntry_Type                      = &PySTEntry_Type;
    storage->p_PySeqIter_Type                      = &PySeqIter_Type;
    storage->p_PySet_Type                          = &PySet_Type;
    storage->p_PySlice_Type                        = &PySlice_Type;
    storage->p_PyStaticMethod_Type                 = &PyStaticMethod_Type;
    storage->p_PyString_Type                       = &PyString_Type;
    storage->p_PySuper_Type                        = &PySuper_Type;
    storage->p_PyTraceBack_Type                    = &PyTraceBack_Type;
    storage->p_PyTuple_Type                        = &PyTuple_Type;
    storage->p_PyType_Type                         = &PyType_Type;
    storage->p_PyUnicode_Type                      = &PyUnicode_Type;
    storage->p_PyWrapperDescr_Type                 = &PyWrapperDescr_Type;
    storage->p_Py_DebugFlag                        = &Py_DebugFlag;
    storage->p_Py_DivisionWarningFlag              = &Py_DivisionWarningFlag;
    storage->p_Py_FileSystemDefaultEncoding        = &Py_FileSystemDefaultEncoding;
    storage->p_Py_FrozenFlag                       = &Py_FrozenFlag;
    storage->p_Py_IgnoreEnvironmentFlag            = &Py_IgnoreEnvironmentFlag;
    storage->p_Py_InteractiveFlag                  = &Py_InteractiveFlag;
    storage->p_Py_NoSiteFlag                       = &Py_NoSiteFlag;
    storage->p_Py_OptimizeFlag                     = &Py_OptimizeFlag;
    storage->p_Py_TabcheckFlag                     = &Py_TabcheckFlag;
    storage->p_Py_UnicodeFlag                      = &Py_UnicodeFlag;
    storage->p_Py_UseClassExceptionsFlag           = &Py_UseClassExceptionsFlag;
    storage->p_Py_VerboseFlag                      = &Py_VerboseFlag;
    storage->p__PyLong_DigitValue                  = &_PyLong_DigitValue;
    storage->p__PyOS_ReadlineTState                = &_PyOS_ReadlineTState;
    storage->p__PyOS_optarg                        = &_PyOS_optarg;
    storage->p__PyOS_opterr                        = &_PyOS_opterr;
    storage->p__PyOS_optind                        = &_PyOS_optind;
    storage->p__PyParser_TokenNames                = &_PyParser_TokenNames;
    storage->p__PyThreadState_Current              = &_PyThreadState_Current;
    storage->p__PyThreadState_GetFrame             = &_PyThreadState_GetFrame;
    storage->p__PyTrash_delete_later               = &_PyTrash_delete_later;
    storage->p__PyTrash_delete_nesting             = &_PyTrash_delete_nesting;
    storage->p__PyWeakref_CallableProxyType        = &_PyWeakref_CallableProxyType;
    storage->p__PyWeakref_ProxyType                = &_PyWeakref_ProxyType;
    storage->p__PyWeakref_RefType                  = &_PyWeakref_RefType;
    storage->p__Py_CheckInterval                   = &_Py_CheckInterval;
    storage->p__Py_CheckRecursionLimit             = &_Py_CheckRecursionLimit;
    storage->p__Py_EllipsisObject                  = &_Py_EllipsisObject;
    storage->p__Py_NoneStruct                      = &_Py_NoneStruct;
    storage->p__Py_NotImplementedStruct            = &_Py_NotImplementedStruct;
    storage->p__Py_PackageContext                  = &_Py_PackageContext;
    storage->p__Py_QnewFlag                        = &_Py_QnewFlag;
    storage->p__Py_SwappedOp                       = &_Py_SwappedOp;
    storage->p__Py_Ticker                          = &_Py_Ticker;
    storage->p__Py_TrueStruct                      = &_Py_TrueStruct;
    storage->p__Py_ZeroStruct                      = &_Py_ZeroStruct;
}
