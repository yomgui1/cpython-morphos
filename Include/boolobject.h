/* Boolean object interface */

#ifndef Py_BOOLOBJECT_H
#define Py_BOOLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


typedef PyIntObject PyBoolObject;

extern DL_IMPORT(PyTypeObject) PyBool_Type;

#define PyBool_Check(x) ((x)->ob_type == &PyBool_Type)

/* Py_False and Py_True are the only two bools in existence.
Don't forget to apply Py_INCREF() when returning either!!! */

/* Don't use these directly */
extern DL_IMPORT(PyIntObject) _Py_ZeroStruct, _Py_TrueStruct;

/* Use these macros */
#define Py_False ((PyObject *) &_Py_ZeroStruct)
#define Py_True ((PyObject *) &_Py_TrueStruct)

/* Function to return a bool from a C long */
extern DL_IMPORT(PyObject *) PyBool_FromLong(long);

#ifdef __cplusplus
}
#endif
#endif /* !Py_BOOLOBJECT_H */
