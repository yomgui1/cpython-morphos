#ifndef PYTHON_PROTOS_H
#define PYTHON_PROTOS_H

#include <Python.h>

#if 0 /* don't use it... it's already given by Python.h (it's just for cvinclude.pl) */
extern int PyAST_Compile (void);
extern int PyAST_FromNode (void);
extern int PyArena_AddPyObject (void);
extern int PyArena_Free (void);
extern int PyArena_Malloc (void);
extern int PyArena_New (void);
extern int PyArg_Parse (void);
extern int PyArg_VaParse (void);
extern int PyArg_VaParseTupleAndKeywords (void);
extern int PyBool_FromLong (void);
extern int PyBuffer_FromMemory (void);
extern int PyBuffer_FromObject (void);
extern int PyBuffer_FromReadWriteMemory (void);
extern int PyBuffer_FromReadWriteObject (void);
extern int PyBuffer_New (void);
extern int PyCFunction_Call (void);
extern int PyCFunction_Fini (void);
extern int PyCFunction_GetFlags (void);
extern int PyCFunction_GetFunction (void);
extern int PyCFunction_GetSelf (void);
extern int PyCFunction_New (void);
extern int PyCFunction_NewEx (void);
extern int PyCObject_AsVoidPtr (void);
extern int PyCObject_FromVoidPtr (void);
extern int PyCObject_FromVoidPtrAndDesc (void);
extern int PyCObject_GetDesc (void);
extern int PyCObject_Import (void);
extern int PyCObject_SetVoidPtr (void);
extern int PyCallIter_New (void);
extern int PyCallable_Check (void);
extern int PyCell_Get (void);
extern int PyCell_New (void);
extern int PyCell_Set (void);
extern int PyClassMethod_New (void);
extern int PyClass_IsSubclass (void);
extern int PyClass_New (void);
extern int PyCode_Addr2Line (void);
extern PyCodeObject * PyCode_New(int argcount, int kwonlyargcount,
        int nlocals, int stacksize, int flags,
        PyObject *code, PyObject *consts, PyObject *names,
        PyObject *varnames, PyObject *freevars, PyObject *cellvars,
        PyObject *filename, PyObject *name, int firstlineno,
        PyObject *lnotab);
extern int PyCodec_BackslashReplaceErrors (void);
extern int PyCodec_Decode (void);
extern int PyCodec_Decoder (void);
extern int PyCodec_Encode (void);
extern int PyCodec_Encoder (void);
extern int PyCodec_IgnoreErrors (void);
extern int PyCodec_IncrementalDecoder (void);
extern int PyCodec_IncrementalEncoder (void);
extern int PyCodec_LookupError (void);
extern int PyCodec_Register (void);
extern int PyCodec_RegisterError (void);
extern int PyCodec_ReplaceErrors (void);
extern int PyCodec_StreamReader (void);
extern int PyCodec_StreamWriter (void);
extern int PyCodec_StrictErrors (void);
extern int PyCodec_XMLCharRefReplaceErrors (void);
extern int PyComplex_AsCComplex (void);
extern int PyComplex_FromCComplex (void);
extern int PyComplex_FromDoubles (void);
extern int PyComplex_ImagAsDouble (void);
extern int PyComplex_RealAsDouble (void);
extern int PyDescr_NewClassMethod (void);
extern int PyDescr_NewGetSet (void);
extern int PyDescr_NewMember (void);
extern int PyDescr_NewMethod (void);
extern int PyDescr_NewWrapper (void);
extern int PyDictProxy_New (void);
extern int PyDict_Clear (void);
extern int PyDict_Contains (void);
extern int PyDict_Copy (void);
extern int PyDict_DelItem (void);
extern int PyDict_DelItemString (void);
extern int PyDict_GetItem (void);
extern int PyDict_GetItemString (void);
extern int PyDict_Items (void);
extern int PyDict_Keys (void);
extern int PyDict_Merge (void);
extern int PyDict_MergeFromSeq2 (void);
extern int PyDict_New (void);
extern int PyDict_Next (void);
extern int PyDict_SetItem (void);
extern int PyDict_SetItemString (void);
extern int PyDict_Size (void);
extern int PyDict_Update (void);
extern int PyDict_Values (void);
extern int PyErr_BadArgument (void);
extern int PyErr_BadInternalCall (void);
extern int PyErr_CheckSignals (void);
extern int PyErr_Clear (void);
extern int PyErr_Display (void);
extern int PyErr_ExceptionMatches (void);
extern int PyErr_Fetch (void);
extern int PyErr_GivenExceptionMatches (void);
extern int PyErr_NewException (void);
extern int PyErr_NoMemory (void);
extern int PyErr_NormalizeException (void);
extern int PyErr_Occurred (void);
extern int PyErr_Print (void);
extern int PyErr_PrintEx (void);
extern int PyErr_ProgramText (void);
extern int PyErr_Restore (void);
extern int PyErr_SetFromErrno (void);
extern int PyErr_SetFromErrnoWithFilename (void);
extern int PyErr_SetFromErrnoWithFilenameObject (void);
extern int PyErr_SetInterrupt (void);
extern int PyErr_SetNone (void);
extern int PyErr_SetObject (void);
extern int PyErr_SetString (void);
extern int PyErr_SyntaxLocation (void);
extern int PyErr_WarnEx (void);
extern int PyErr_WarnExplicit (void);
extern int PyErr_WriteUnraisable (void);
extern int PyEval_AcquireLock (void);
extern int PyEval_AcquireThread (void);
extern int PyEval_CallObjectWithKeywords (void);
extern int PyEval_EvalCode (void);
extern int PyEval_EvalCodeEx (void);
extern int PyEval_EvalFrame (void);
extern int PyEval_EvalFrameEx (void);
extern int PyEval_GetBuiltins (void);
extern int PyEval_GetCallStats (void);
extern int PyEval_GetFrame (void);
extern int PyEval_GetFuncDesc (void);
extern int PyEval_GetFuncName (void);
extern int PyEval_GetGlobals (void);
extern int PyEval_GetLocals (void);
extern int PyEval_GetRestricted (void);
extern int PyEval_InitThreads (void);
extern int PyEval_MergeCompilerFlags (void);
extern int PyEval_ReInitThreads (void);
extern int PyEval_ReleaseLock (void);
extern int PyEval_ReleaseThread (void);
extern int PyEval_RestoreThread (void);
extern int PyEval_SaveThread (void);
extern int PyEval_SetProfile (void);
extern int PyEval_SetTrace (void);
extern int PyEval_ThreadsInitialized (void);
extern int PyFile_AsFile (void);
extern int PyFile_FromFile (void);
extern int PyFile_FromString (void);
extern int PyFile_GetLine (void);
extern int PyFile_Name (void);
extern int PyFile_SetBufSize (void);
extern int PyFile_SetEncoding (void);
extern int PyFile_SoftSpace (void);
extern int PyFile_WriteObject (void);
extern int PyFile_WriteString (void);
extern int PyFloat_AsDouble (void);
extern int PyFloat_AsReprString (void);
extern int PyFloat_AsString (void);
extern int PyFloat_Fini (void);
extern int PyFloat_FromDouble (void);
extern int PyFloat_FromString (void);
extern int PyFrame_BlockPop (void);
extern int PyFrame_BlockSetup (void);
extern int PyFrame_FastToLocals (void);
extern int PyFrame_Fini (void);
extern int PyFrame_LocalsToFast (void);
extern int PyFrame_New (void);
extern int PyFrozenSet_New (void);
extern int PyFunction_GetClosure (void);
extern int PyFunction_GetCode (void);
extern int PyFunction_GetDefaults (void);
extern int PyFunction_GetGlobals (void);
extern int PyFunction_GetModule (void);
extern int PyFunction_New (void);
extern int PyFunction_SetClosure (void);
extern int PyFunction_SetDefaults (void);
extern int PyFuture_FromAST (void);
extern int PyGC_Collect (void);
extern int PyGILState_Ensure (void);
extern int PyGILState_GetThisThreadState (void);
extern int PyGILState_Release (void);
extern int PyGen_NeedsFinalizing (void);
extern int PyGen_New (void);
extern int PyImport_AddModule (void);
extern int PyImport_AppendInittab (void);
extern int PyImport_Cleanup (void);
extern int PyImport_ExecCodeModule (void);
extern int PyImport_ExecCodeModuleEx (void);
extern int PyImport_ExtendInittab (void);
extern int PyImport_GetMagicNumber (void);
extern int PyImport_GetModuleDict (void);
extern int PyImport_Import (void);
extern int PyImport_ImportFrozenModule (void);
extern int PyImport_ImportModule (void);
extern int PyImport_ImportModuleLevel (void);
extern int PyImport_ReloadModule (void);
extern int PyInstance_New (void);
extern int PyInstance_NewRaw (void);
extern int PyInt_AsLong (void);
extern int PyInt_AsSsize_t (void);
extern int PyInt_AsUnsignedLongLongMask (void);
extern int PyInt_AsUnsignedLongMask (void);
extern int PyInt_Fini (void);
extern int PyInt_FromLong (void);
extern int PyInt_FromSize_t (void);
extern int PyInt_FromSsize_t (void);
extern int PyInt_FromString (void);
extern int PyInt_FromUnicode (void);
extern int PyInt_GetMax (void);
extern int PyInterpreterState_Clear (void);
extern int PyInterpreterState_Delete (void);
extern int PyInterpreterState_Head (void);
extern int PyInterpreterState_New (void);
extern int PyInterpreterState_Next (void);
extern int PyInterpreterState_ThreadHead (void);
extern int PyIter_Next (void);
extern int PyList_Append (void);
extern int PyList_AsTuple (void);
extern int PyList_Fini (void);
extern int PyList_GetItem (void);
extern int PyList_GetSlice (void);
extern int PyList_Insert (void);
extern int PyList_New (void);
extern int PyList_Reverse (void);
extern int PyList_SetItem (void);
extern int PyList_SetSlice (void);
extern int PyList_Size (void);
extern int PyList_Sort (void);
extern int PyLong_AsDouble (void);
extern int PyLong_AsLong (void);
extern int PyLong_AsLongLong (void);
extern int PyLong_AsUnsignedLong (void);
extern int PyLong_AsUnsignedLongLong (void);
extern int PyLong_AsUnsignedLongLongMask (void);
extern int PyLong_AsUnsignedLongMask (void);
extern int PyLong_AsVoidPtr (void);
extern int PyLong_FromDouble (void);
extern int PyLong_FromLong (void);
extern int PyLong_FromLongLong (void);
extern int PyLong_FromString (void);
extern int PyLong_FromUnicode (void);
extern int PyLong_FromUnsignedLong (void);
extern int PyLong_FromUnsignedLongLong (void);
extern int PyLong_FromVoidPtr (void);
extern int PyMapping_Check (void);
extern int PyMapping_GetItemString (void);
extern int PyMapping_HasKey (void);
extern int PyMapping_HasKeyString (void);
extern int PyMapping_Length (void);
extern int PyMapping_SetItemString (void);
extern int PyMapping_Size (void);
extern int PyMarshal_ReadLastObjectFromFile (void);
extern int PyMarshal_ReadLongFromFile (void);
extern int PyMarshal_ReadObjectFromFile (void);
extern int PyMarshal_ReadObjectFromString (void);
extern int PyMarshal_ReadShortFromFile (void);
extern int PyMarshal_WriteLongToFile (void);
extern int PyMarshal_WriteObjectToFile (void);
extern int PyMarshal_WriteObjectToString (void);
extern int PyMem_Free (void);
extern int PyMem_Malloc (void);
extern int PyMem_Realloc (void);
extern int PyMember_Get (void);
extern int PyMember_GetOne (void);
extern int PyMember_Set (void);
extern int PyMember_SetOne (void);
extern int PyMethod_Class (void);
extern int PyMethod_Fini (void);
extern int PyMethod_Function (void);
extern int PyMethod_New (void);
extern int PyMethod_Self (void);
extern int PyModule_AddIntConstant (void);
extern int PyModule_AddObject (void);
extern int PyModule_AddStringConstant (void);
extern int PyModule_GetDict (void);
extern int PyModule_GetFilename (void);
extern int PyModule_GetName (void);
extern int PyModule_New (void);
extern int PyMorphOS_AddTermFunc (void);
extern int PyMorphOS_GetFullPath (void);
extern int PyMorphOS_GetGVars (void);
extern int PyMorphOS_HandleArgv (void);
extern int PyMorphOS_InitGVars (void);
extern int PyMorphOS_InitThread (void);
extern int PyMorphOS_SetConfigA (void);
extern int PyMorphOS_Term (void);
extern int PyMorphOS_TermThread (void);
extern int PyNode_AddChild (void);
extern int PyNode_Compile (void);
extern int PyNode_Free (void);
extern int PyNode_ListTree (void);
extern int PyNode_New (void);
extern int PyNumber_Absolute (void);
extern int PyNumber_Add (void);
extern int PyNumber_And (void);
extern int PyNumber_AsSsize_t (void);
extern int PyNumber_Check (void);
extern int PyNumber_Coerce (void);
extern int PyNumber_CoerceEx (void);
extern int PyNumber_Divide (void);
extern int PyNumber_Divmod (void);
extern int PyNumber_Float (void);
extern int PyNumber_FloorDivide (void);
extern int PyNumber_InPlaceAdd (void);
extern int PyNumber_InPlaceAnd (void);
extern int PyNumber_InPlaceDivide (void);
extern int PyNumber_InPlaceFloorDivide (void);
extern int PyNumber_InPlaceLshift (void);
extern int PyNumber_InPlaceMultiply (void);
extern int PyNumber_InPlaceOr (void);
extern int PyNumber_InPlacePower (void);
extern int PyNumber_InPlaceRemainder (void);
extern int PyNumber_InPlaceRshift (void);
extern int PyNumber_InPlaceSubtract (void);
extern int PyNumber_InPlaceTrueDivide (void);
extern int PyNumber_InPlaceXor (void);
extern int PyNumber_Index (void);
extern int PyNumber_Int (void);
extern int PyNumber_Invert (void);
extern int PyNumber_Long (void);
extern int PyNumber_Lshift (void);
extern int PyNumber_Multiply (void);
extern int PyNumber_Negative (void);
extern int PyNumber_Or (void);
extern int PyNumber_Positive (void);
extern int PyNumber_Power (void);
extern int PyNumber_Remainder (void);
extern int PyNumber_Rshift (void);
extern int PyNumber_Subtract (void);
extern int PyNumber_TrueDivide (void);
extern int PyNumber_Xor (void);
extern int PyOS_AfterFork (void);
extern int PyOS_FiniInterrupts (void);
extern int PyOS_InitInterrupts (void);
extern int PyOS_InterruptOccurred (void);
extern int PyOS_Readline (void);
extern int PyOS_ascii_atof (void);
extern int PyOS_ascii_formatd (void);
extern int PyOS_ascii_strtod (void);
extern int PyOS_getsig (void);
extern int PyOS_setsig (void);
extern int PyOS_strtol (void);
extern int PyOS_strtoul (void);
extern int PyOS_vsnprintf (void);
extern int PyObject_AsCharBuffer (void);
extern int PyObject_AsFileDescriptor (void);
extern int PyObject_AsReadBuffer (void);
extern int PyObject_AsWriteBuffer (void);
extern int PyObject_Call (void);
extern int PyObject_CallObject (void);
extern int PyObject_CheckReadBuffer (void);
extern int PyObject_ClearWeakRefs (void);
extern int PyObject_Cmp (void);
extern int PyObject_Compare (void);
extern int PyObject_DelItem (void);
extern int PyObject_DelItemString (void);
extern int PyObject_Dir (void);
extern int PyObject_Free (void);
extern int PyObject_GC_Del (void);
extern int PyObject_GC_Track (void);
extern int PyObject_GC_UnTrack (void);
extern int PyObject_GenericGetAttr (void);
extern int PyObject_GenericSetAttr (void);
extern int PyObject_GetAttr (void);
extern int PyObject_GetAttrString (void);
extern int PyObject_GetItem (void);
extern int PyObject_GetIter (void);
extern int PyObject_HasAttr (void);
extern int PyObject_HasAttrString (void);
extern int PyObject_Hash (void);
extern int PyObject_Init (void);
extern int PyObject_InitVar (void);
extern int PyObject_IsInstance (void);
extern int PyObject_IsSubclass (void);
extern int PyObject_IsTrue (void);
extern int PyObject_Length (void);
extern int PyObject_Malloc (void);
extern int PyObject_Not (void);
extern int PyObject_Print (void);
extern int PyObject_Realloc (void);
extern int PyObject_Repr (void);
extern int PyObject_RichCompare (void);
extern int PyObject_RichCompareBool (void);
extern int PyObject_SelfIter (void);
extern int PyObject_SetAttr (void);
extern int PyObject_SetAttrString (void);
extern int PyObject_SetItem (void);
extern int PyObject_Size (void);
extern int PyObject_Str (void);
extern int PyObject_Type (void);
extern int PyObject_Unicode (void);
extern struct _mod * PyParser_ASTFromFile(FILE *, const char *, 
        const char*, int, 
        char *, char *,
        PyCompilerFlags *, int *,
        PyArena *);
extern int PyParser_ASTFromString (void);
extern node * PyParser_ParseFileFlags(FILE *, const char *, 
        const char*, grammar *,
        int, char *, char *,
        perrdetail *, int);
extern int PyParser_ParseString (void);
extern int PyParser_ParseStringFlags (void);
extern int PyParser_ParseStringFlagsFilename (void);
extern int PyParser_SetError (void);
extern int PyParser_SimpleParseFileFlags (void);
extern int PyParser_SimpleParseStringFlags (void);
extern int PyRun_AnyFileExFlags (void);
extern int PyRun_AnyFileFlags (void);
extern int PyRun_FileExFlags (void);
extern int PyRun_InteractiveLoopFlags (void);
extern int PyRun_InteractiveOneFlags (void);
extern int PyRun_SimpleFileExFlags (void);
extern int PyRun_SimpleStringFlags (void);
extern int PyRun_StringFlags (void);
extern int PyST_GetScope (void);
extern int PySeqIter_New (void);
extern int PySequence_Check (void);
extern int PySequence_Concat (void);
extern int PySequence_Contains (void);
extern int PySequence_Count (void);
extern int PySequence_DelItem (void);
extern int PySequence_DelSlice (void);
extern int PySequence_Fast (void);
extern int PySequence_GetItem (void);
extern int PySequence_GetSlice (void);
extern int PySequence_In (void);
extern int PySequence_InPlaceConcat (void);
extern int PySequence_InPlaceRepeat (void);
extern int PySequence_Index (void);
extern int PySequence_Length (void);
extern int PySequence_List (void);
extern int PySequence_Repeat (void);
extern int PySequence_SetItem (void);
extern int PySequence_SetSlice (void);
extern int PySequence_Size (void);
extern int PySequence_Tuple (void);
extern int PySet_Add (void);
extern int PySet_Clear (void);
extern int PySet_Contains (void);
extern int PySet_Discard (void);
extern int PySet_Fini (void);
extern int PySet_New (void);
extern int PySet_Pop (void);
extern int PySet_Size (void);
extern int PySlice_GetIndices (void);
extern int PySlice_GetIndicesEx (void);
extern int PySlice_New (void);
extern int PyStaticMethod_New (void);
extern int PyString_AsDecodedObject (void);
extern int PyString_AsDecodedString (void);
extern int PyString_AsEncodedObject (void);
extern int PyString_AsEncodedString (void);
extern int PyString_AsString (void);
extern int PyString_AsStringAndSize (void);
extern int PyString_Concat (void);
extern int PyString_ConcatAndDel (void);
extern int PyString_Decode (void);
extern int PyString_DecodeEscape (void);
extern int PyString_Encode (void);
extern int PyString_Fini (void);
extern int PyString_Format (void);
extern int PyString_FromFormatV (void);
extern int PyString_FromString (void);
extern int PyString_FromStringAndSize (void);
extern int PyString_InternFromString (void);
extern int PyString_InternImmortal (void);
extern int PyString_InternInPlace (void);
extern int PyString_Repr (void);
extern int PyString_Size (void);
extern int PyStructSequence_InitType (void);
extern int PyStructSequence_New (void);
extern int PySymtable_Build (void);
extern int PySymtable_Free (void);
extern int PySymtable_Lookup (void);
extern int PySys_AddWarnOption (void);
extern int PySys_GetFile (void);
extern int PySys_GetObject (void);
extern int PySys_ResetWarnOptions (void);
extern int PySys_SetArgv (void);
extern int PySys_SetObject (void);
extern int PySys_SetPath (void);
extern int PyThreadState_Clear (void);
extern int PyThreadState_Delete (void);
extern int PyThreadState_DeleteCurrent (void);
extern int PyThreadState_Get (void);
extern int PyThreadState_GetDict (void);
extern int PyThreadState_New (void);
extern int PyThreadState_Next (void);
extern int PyThreadState_SetAsyncExc (void);
extern int PyThreadState_Swap (void);
extern int PyThread_ReInitTLS (void);
extern int PyThread_acquire_lock (void);
extern int PyThread_allocate_lock (void);
extern int PyThread_create_key (void);
extern int PyThread_delete_key (void);
extern int PyThread_delete_key_value (void);
extern int PyThread_exit_thread (void);
extern int PyThread_free_lock (void);
extern int PyThread_get_key_value (void);
extern int PyThread_get_stacksize (void);
extern int PyThread_get_thread_ident (void);
extern int PyThread_init_thread (void);
extern int PyThread_release_lock (void);
extern int PyThread_set_key_value (void);
extern int PyThread_set_stacksize (void);
extern int PyThread_start_new_thread (void);
extern int PyToken_OneChar (void);
extern int PyToken_ThreeChars (void);
extern int PyToken_TwoChars (void);
extern int PyTraceBack_Here (void);
extern int PyTraceBack_Print (void);
extern int PyTuple_Fini (void);
extern int PyTuple_GetItem (void);
extern int PyTuple_GetSlice (void);
extern int PyTuple_New (void);
extern int PyTuple_SetItem (void);
extern int PyTuple_Size (void);
extern int PyType_GenericAlloc (void);
extern int PyType_GenericNew (void);
extern int PyType_IsSubtype (void);
extern int PyType_Ready (void);
extern int PyUnicodeDecodeError_Create (void);
extern int PyUnicodeDecodeError_GetEncoding (void);
extern int PyUnicodeDecodeError_GetEnd (void);
extern int PyUnicodeDecodeError_GetObject (void);
extern int PyUnicodeDecodeError_GetReason (void);
extern int PyUnicodeDecodeError_GetStart (void);
extern int PyUnicodeDecodeError_SetEnd (void);
extern int PyUnicodeDecodeError_SetReason (void);
extern int PyUnicodeDecodeError_SetStart (void);
extern int PyUnicodeEncodeError_Create (void);
extern int PyUnicodeEncodeError_GetEncoding (void);
extern int PyUnicodeEncodeError_GetEnd (void);
extern int PyUnicodeEncodeError_GetObject (void);
extern int PyUnicodeEncodeError_GetReason (void);
extern int PyUnicodeEncodeError_GetStart (void);
extern int PyUnicodeEncodeError_SetEnd (void);
extern int PyUnicodeEncodeError_SetReason (void);
extern int PyUnicodeEncodeError_SetStart (void);
extern int PyUnicodeTranslateError_Create (void);
extern int PyUnicodeTranslateError_GetEnd (void);
extern int PyUnicodeTranslateError_GetObject (void);
extern int PyUnicodeTranslateError_GetReason (void);
extern int PyUnicodeTranslateError_GetStart (void);
extern int PyUnicodeTranslateError_SetEnd (void);
extern int PyUnicodeTranslateError_SetReason (void);
extern int PyUnicodeTranslateError_SetStart (void);
extern int PyUnicode_BuildEncodingMap (void);
extern int PyUnicode_DecodeUTF7 (void);
extern int PyUnicode_EncodeUTF7 (void);
extern int PyWeakref_GetObject (void);
extern int PyWeakref_NewProxy (void);
extern int PyWeakref_NewRef (void);
extern int PyWrapper_New (void);
extern int Py_AddPendingCall (void);
extern int Py_AtExit (void);
extern int Py_CompileStringFlags (void);
extern int Py_DecRef (void);
extern int Py_EndInterpreter (void);
extern int Py_Exit (void);
extern int Py_FatalError (void);
extern int Py_FdIsInteractive (void);
extern int Py_Finalize (void);
extern int Py_FindMethod (void);
extern int Py_FindMethodInChain (void);
extern int Py_FlushLine (void);
extern int Py_GetBuildInfo (void);
extern int Py_GetCompiler (void);
extern int Py_GetCopyright (void);
extern int Py_GetExecPrefix (void);
extern int Py_GetPath (void);
extern int Py_GetPlatform (void);
extern int Py_GetPrefix (void);
extern int Py_GetProgramFullPath (void);
extern int Py_GetProgramName (void);
extern int Py_GetPythonHome (void);
extern int Py_GetRecursionLimit (void);
extern int Py_GetVersion (void);
extern int Py_IncRef (void);
extern int Py_InitModule4 (void);
extern int Py_Initialize (void);
extern int Py_InitializeEx (void);
extern int Py_IsInitialized (void);
extern int Py_Main (void);
extern int Py_MakePendingCalls (void);
extern int Py_NewInterpreter (void);
extern int Py_ReprEnter (void);
extern int Py_ReprLeave (void);
extern int Py_SetProgramName (void);
extern int Py_SetPythonHome (void);
extern int Py_SetRecursionLimit (void);
extern int Py_SubversionRevision (void);
extern int Py_SubversionShortBranch (void);
extern int Py_SymtableString (void);
extern int Py_VaBuildValue (void);
extern int _PyArg_NoKeywords (void);
extern int _PyArg_Parse_SizeT (void);
extern int _PyArg_VaParseTupleAndKeywords_SizeT (void);
extern int _PyArg_VaParse_SizeT (void);
extern int _PyBuiltin_Init (void);
extern int _PyCodec_Lookup (void);
extern int _PyDict_Contains (void);
extern int _PyDict_Next (void);
extern int _PyErr_BadInternalCall (void);
extern int _PyEval_CallTracing (void);
extern int _PyEval_SliceIndex (void);
extern int _PyExc_Fini (void);
extern int _PyExc_Init (void);
extern int _PyFloat_Init (void);
extern int _PyFloat_Pack4 (void);
extern int _PyFloat_Pack8 (void);
extern int _PyFloat_Unpack4 (void);
extern int _PyFloat_Unpack8 (void);
extern int _PyFrame_Init (void);
extern int _PyImportHooks_Init (void);
extern int _PyImport_FindModule (void);
extern int _PyImport_Fini (void);
extern int _PyImport_Init (void);
extern int _PyImport_IsScript (void);
extern int _PyImport_ReInitLock (void);
extern int _PyInstance_Lookup (void);
extern int _PyInt_Init (void);
extern int _PyList_Extend (void);
extern int _PyLong_AsByteArray (void);
extern int _PyLong_Copy (void);
extern int _PyLong_FromByteArray (void);
extern int _PyLong_New (void);
extern int _PyLong_NumBits (void);
extern int _PyLong_Sign (void);
extern int _PyModule_Clear (void);
extern int _PyOS_GetOpt (void);
extern int _PyObject_Dump (void);
extern int _PyObject_GC_Malloc (void);
extern int _PyObject_GC_New (void);
extern int _PyObject_GC_NewVar (void);
extern int _PyObject_GC_Resize (void);
extern int _PyObject_GetDictPtr (void);
extern int _PyObject_LengthHint (void);
extern int _PyObject_New (void);
extern int _PyObject_NewVar (void);
extern int _PyObject_Str (void);
extern int _PySequence_IterSearch (void);
extern int _PySet_Next (void);
extern int _PySet_NextEntry (void);
extern int _PySet_Update (void);
extern int _PySlice_FromIndices (void);
extern int _PyString_Eq (void);
extern int _PyString_FormatLong (void);
extern int _PyString_Join (void);
extern int _PyString_Resize (void);
extern int _PySys_Init (void);
extern int _PyThread_CurrentFrames (void);
extern int _PyTime_DoubleToTimet (void);
extern int _PyTrash_deposit_object (void);
extern int _PyTrash_destroy_chain (void);
extern int _PyTuple_Resize (void);
extern int _PyType_Lookup (void);
extern int _PyUnicode_XStrip (void);
extern int _PyWeakref_ClearRef (void);
extern int _PyWeakref_GetWeakrefCount (void);
extern int _Py_CheckRecursiveCall (void);
extern int _Py_HashDouble (void);
extern int _Py_HashPointer (void);
extern int _Py_Mangle (void);
extern int _Py_ReleaseInternedStrings (void);
extern int _Py_VaBuildValue_SizeT (void);
extern int _Py_c_diff (void);
extern int _Py_c_neg (void);
extern int _Py_c_pow (void);
extern int _Py_c_prod (void);
extern int _Py_c_quot (void);
extern int _Py_c_sum (void);
extern int _Py_svnversion (void);
extern int PyBuffer_FillContiguousStrides (void);
extern int PyBuffer_FillInfo (void);
extern int PyBuffer_FromContiguous (void);
extern int PyBuffer_GetPointer (void);
extern int PyBuffer_IsContiguous (void);
extern int PyBuffer_Release (void);
extern int PyBuffer_ToContiguous (void);
extern int PyByteArray_AsString (void);
extern int PyByteArray_Concat (void);
extern int PyByteArray_Fini (void);
extern int PyByteArray_FromObject (void);
extern int PyByteArray_FromStringAndSize (void);
extern int PyByteArray_Init (void);
extern int PyByteArray_Resize (void);
extern int PyByteArray_Size (void);
extern int PyCFunction_ClearFreeList (void);
extern int PyCapsule_GetContext (void);
extern int PyCapsule_GetDestructor (void);
extern int PyCapsule_GetName (void);
extern int PyCapsule_GetPointer (void);
extern int PyCapsule_Import (void);
extern int PyCapsule_IsValid (void);
extern int PyCapsule_New (void);
extern int PyCapsule_SetContext (void);
extern int PyCapsule_SetDestructor (void);
extern int PyCapsule_SetName (void);
extern int PyCapsule_SetPointer (void);
extern int PyCode_Optimize (void);
extern int PyDict_Fini (void);
extern int PyErr_NewExceptionWithDoc (void);
extern int PyFile_DecUseCount (void);
extern int PyFile_IncUseCount (void);
extern int PyFile_SetEncodingAndErrors (void);
extern int PyFloat_ClearFreeList (void);
extern int PyFloat_GetInfo (void);
extern int PyFloat_GetMax (void);
extern int PyFloat_GetMin (void);
extern int PyFrame_ClearFreeList (void);
extern int PyFrame_GetLineNumber (void);
extern int PyImport_GetImporter (void);
extern int PyImport_ImportModuleNoBlock (void);
extern int PyInt_ClearFreeList (void);
extern int PyLong_AsLongAndOverflow (void);
extern int PyLong_AsLongLongAndOverflow (void);
extern int PyLong_AsSsize_t (void);
extern int PyLong_FromSize_t (void);
extern int PyLong_FromSsize_t (void);
extern int PyLong_GetInfo (void);
extern int PyMemoryView_FromBuffer (void);
extern int PyMemoryView_FromObject (void);
extern int PyMemoryView_GetContiguous (void);
extern int PyMethod_ClearFreeList (void);
extern int PyNumber_ToBase (void);
extern int PyOS_double_to_string (void);
extern int PyOS_mystricmp (void);
extern int PyOS_mystrnicmp (void);
extern int PyOS_string_to_double (void);
extern int PyObject_CopyData (void);
extern int PyObject_Format (void);
extern int PyObject_GetBuffer (void);
extern int PyObject_HashNotImplemented (void);
extern node * PyParser_ParseFileFlagsEx(FILE *, const char *,
        const char*, grammar *,
        int, char *, char *,
        perrdetail *, int *);
extern int PyParser_ParseStringFlagsFilenameEx (void);
extern int PySys_HasWarnOptions (void);
extern int PySys_SetArgvEx (void);
extern int PyTuple_ClearFreeList (void);
extern int PyType_ClearCache (void);
extern int PyType_Modified (void);
extern int PyUnicodeUCS2_AsASCIIString (void);
extern int PyUnicodeUCS2_AsCharmapString (void);
extern int PyUnicodeUCS2_AsEncodedObject (void);
extern int PyUnicodeUCS2_AsEncodedString (void);
extern int PyUnicodeUCS2_AsLatin1String (void);
extern int PyUnicodeUCS2_AsRawUnicodeEscapeString (void);
extern int PyUnicodeUCS2_AsUTF16String (void);
extern int PyUnicodeUCS2_AsUTF32String (void);
extern int PyUnicodeUCS2_AsUTF8String (void);
extern int PyUnicodeUCS2_AsUnicode (void);
extern int PyUnicodeUCS2_AsUnicodeEscapeString (void);
extern int PyUnicodeUCS2_AsWideChar (void);
extern int PyUnicodeUCS2_Compare (void);
extern int PyUnicodeUCS2_Concat (void);
extern int PyUnicodeUCS2_Contains (void);
extern int PyUnicodeUCS2_Count (void);
extern int PyUnicodeUCS2_Decode (void);
extern int PyUnicodeUCS2_DecodeASCII (void);
extern int PyUnicodeUCS2_DecodeCharmap (void);
extern int PyUnicodeUCS2_DecodeLatin1 (void);
extern int PyUnicodeUCS2_DecodeRawUnicodeEscape (void);
extern int PyUnicodeUCS2_DecodeUTF16 (void);
extern int PyUnicodeUCS2_DecodeUTF16Stateful (void);
extern int PyUnicodeUCS2_DecodeUTF32 (void);
extern int PyUnicodeUCS2_DecodeUTF32Stateful (void);
extern int PyUnicodeUCS2_DecodeUTF8 (void);
extern int PyUnicodeUCS2_DecodeUTF8Stateful (void);
extern int PyUnicodeUCS2_DecodeUnicodeEscape (void);
extern int PyUnicodeUCS2_Encode (void);
extern int PyUnicodeUCS2_EncodeASCII (void);
extern int PyUnicodeUCS2_EncodeCharmap (void);
extern int PyUnicodeUCS2_EncodeDecimal (void);
extern int PyUnicodeUCS2_EncodeLatin1 (void);
extern int PyUnicodeUCS2_EncodeRawUnicodeEscape (void);
extern int PyUnicodeUCS2_EncodeUTF16 (void);
extern int PyUnicodeUCS2_EncodeUTF32 (void);
extern int PyUnicodeUCS2_EncodeUTF8 (void);
extern int PyUnicodeUCS2_EncodeUnicodeEscape (void);
extern int PyUnicodeUCS2_Find (void);
extern int PyUnicodeUCS2_Format (void);
extern int PyUnicodeUCS2_FromEncodedObject (void);
extern int PyUnicodeUCS2_FromFormat (void);
extern int PyUnicodeUCS2_FromFormatV (void);
extern int PyUnicodeUCS2_FromObject (void);
extern int PyUnicodeUCS2_FromOrdinal (void);
extern int PyUnicodeUCS2_FromString (void);
extern int PyUnicodeUCS2_FromStringAndSize (void);
extern int PyUnicodeUCS2_FromUnicode (void);
extern int PyUnicodeUCS2_FromWideChar (void);
extern int PyUnicodeUCS2_GetDefaultEncoding (void);
extern int PyUnicodeUCS2_GetMax (void);
extern int PyUnicodeUCS2_GetSize (void);
extern int PyUnicodeUCS2_Join (void);
extern int PyUnicodeUCS2_Partition (void);
extern int PyUnicodeUCS2_RPartition (void);
extern int PyUnicodeUCS2_RSplit (void);
extern int PyUnicodeUCS2_Replace (void);
extern int PyUnicodeUCS2_Resize (void);
extern int PyUnicodeUCS2_RichCompare (void);
extern int PyUnicodeUCS2_SetDefaultEncoding (void);
extern int PyUnicodeUCS2_Split (void);
extern int PyUnicodeUCS2_Splitlines (void);
extern int PyUnicodeUCS2_Tailmatch (void);
extern int PyUnicodeUCS2_Translate (void);
extern int PyUnicodeUCS2_TranslateCharmap (void);
extern int PyUnicode_DecodeUTF7Stateful (void);
extern int _PyBytes_FormatAdvanced (void);
extern int _PyCode_CheckLineNumber (void);
extern int _PyComplex_FormatAdvanced (void);
extern int _PyDict_MaybeUntrack (void);
extern int _PyDict_NewPresized (void);
extern int _PyFloat_FormatAdvanced (void);
extern int _PyImport_AcquireLock (void);
extern int _PyImport_ReleaseLock (void);
extern int _PyInt_Format (void);
extern int _PyInt_FormatAdvanced (void);
extern int _PyLong_Format (void);
extern int _PyLong_FormatAdvanced (void);
extern int _PyLong_Frexp (void);
extern int _PyLong_Init (void);
extern int _PyNumber_ConvertIntegralToInt (void);
extern int _PyOS_ResetGetOpt (void);
extern int _PyObject_LookupSpecial (void);
extern int _PyObject_NextNotImplemented (void);
extern int _PyObject_RealIsInstance (void);
extern int _PyObject_RealIsSubclass (void);
extern int _PyRandom_Init (void);
extern int _PyString_InsertThousandsGrouping (void);
extern int _PyThreadState_Init (void);
extern int _PyThreadState_Prealloc (void);
extern int _PyTuple_MaybeUntrack (void);
extern int _PyUnicodeUCS2_AsDefaultEncodedString (void);
extern int _PyUnicodeUCS2_IsAlpha (void);
extern int _PyUnicodeUCS2_IsDecimalDigit (void);
extern int _PyUnicodeUCS2_IsDigit (void);
extern int _PyUnicodeUCS2_IsLinebreak (void);
extern int _PyUnicodeUCS2_IsLowercase (void);
extern int _PyUnicodeUCS2_IsNumeric (void);
extern int _PyUnicodeUCS2_IsTitlecase (void);
extern int _PyUnicodeUCS2_IsUppercase (void);
extern int _PyUnicodeUCS2_IsWhitespace (void);
extern int _PyUnicodeUCS2_ToDecimalDigit (void);
extern int _PyUnicodeUCS2_ToDigit (void);
extern int _PyUnicodeUCS2_ToLowercase (void);
extern int _PyUnicodeUCS2_ToNumeric (void);
extern int _PyUnicodeUCS2_ToTitlecase (void);
extern int _PyUnicodeUCS2_ToUppercase (void);
extern int _PyUnicode_FormatAdvanced (void);
extern int _PyWarnings_Init (void);
extern int _Py_DisplaySourceLine (void);
extern int _Py_add_one_to_index_C (void);
extern int _Py_add_one_to_index_F (void);
extern int _Py_c_abs (void);
extern int _Py_dg_dtoa (void);
extern int _Py_dg_freedtoa (void);
extern int _Py_dg_strtod (void);
extern int _Py_double_round (void);
extern int _Py_hgidentifier (void);
extern int _Py_hgversion (void);
extern int _Py_parse_inf_or_nan (void);
extern int PyCode_NewEmpty (void);
#endif

#if defined(USE_INLINE_STDARG) && !defined(__STRICT_ANSI__)
#include <stdarg.h>
#define PyMorphOS_SetConfig(__p0, ...) 	({ULONG _tags[] = { __VA_ARGS__ }; 	PyMorphOS_SetConfigA(__p0, (struct TagItem *)_tags);})
#endif

#endif /* PYTHON_PROTOS_H */
