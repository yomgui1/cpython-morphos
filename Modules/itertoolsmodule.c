
#include "Python.h"

/* Itertools module written and maintained 
   by Raymond D. Hettinger <python@rcn.com>
   Copyright (c) 2003 Python Software Foundation.
   All rights reserved.
*/

/* dropwhile object **********************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *func;
	PyObject *it;
	long	 start;
} dropwhileobject;

PyTypeObject dropwhile_type;

static PyObject *
dropwhile_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *func, *seq;
	PyObject *it;
	dropwhileobject *lz;

	if (!PyArg_UnpackTuple(args, "dropwhile", 2, 2, &func, &seq))
		return NULL;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* create dropwhileobject structure */
	lz = (dropwhileobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(it);
		return NULL;
	}
	Py_INCREF(func);
	lz->func = func;
	lz->it = it;
	lz->start = 0;

	return (PyObject *)lz;
}

static void
dropwhile_dealloc(dropwhileobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->func);
	Py_XDECREF(lz->it);
	lz->ob_type->tp_free(lz);
}

static int
dropwhile_traverse(dropwhileobject *lz, visitproc visit, void *arg)
{
	int err;

	if (lz->it) {
		err = visit(lz->it, arg);
		if (err)
			return err;
	}
	if (lz->func) {
		err = visit(lz->func, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
dropwhile_next(dropwhileobject *lz)
{
	PyObject *item, *good;
	long ok;

	for (;;) {
		item = PyIter_Next(lz->it);
		if (item == NULL)
			return NULL;
		if (lz->start == 1)
			return item;

		good = PyObject_CallFunctionObjArgs(lz->func, item, NULL);
		if (good == NULL) {
			Py_DECREF(item);
			return NULL;
		}
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
		if (!ok) {
			lz->start = 1;
			return item;
		}
		Py_DECREF(item);
	}
}

static PyObject *
dropwhile_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(dropwhile_doc,
"dropwhile(predicate, iterable) --> dropwhile object\n\
\n\
Drop items from the iterable while predicate(item) is true.\n\
Afterwards, return every element until the iterable is exhausted.");

PyTypeObject dropwhile_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.dropwhile",		 /* tp_name */
	sizeof(dropwhileobject),		 /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)dropwhile_dealloc,	   /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	dropwhile_doc,			   /* tp_doc */
	(traverseproc)dropwhile_traverse,    /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)dropwhile_getiter,	   /* tp_iter */
	(iternextfunc)dropwhile_next,	   /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	dropwhile_new,			 /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* takewhile object **********************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *func;
	PyObject *it;
	long	 stop;
} takewhileobject;

PyTypeObject takewhile_type;

static PyObject *
takewhile_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *func, *seq;
	PyObject *it;
	takewhileobject *lz;

	if (!PyArg_UnpackTuple(args, "takewhile", 2, 2, &func, &seq))
		return NULL;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* create takewhileobject structure */
	lz = (takewhileobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(it);
		return NULL;
	}
	Py_INCREF(func);
	lz->func = func;
	lz->it = it;
	lz->stop = 0;

	return (PyObject *)lz;
}

static void
takewhile_dealloc(takewhileobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->func);
	Py_XDECREF(lz->it);
	lz->ob_type->tp_free(lz);
}

static int
takewhile_traverse(takewhileobject *lz, visitproc visit, void *arg)
{
	int err;

	if (lz->it) {
		err = visit(lz->it, arg);
		if (err)
			return err;
	}
	if (lz->func) {
		err = visit(lz->func, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
takewhile_next(takewhileobject *lz)
{
	PyObject *item, *good;
	long ok;

	if (lz->stop == 1)
		return NULL;

	item = PyIter_Next(lz->it);
	if (item == NULL)
		return NULL;

	good = PyObject_CallFunctionObjArgs(lz->func, item, NULL);
	if (good == NULL) {
		Py_DECREF(item);
		return NULL;
	}
	ok = PyObject_IsTrue(good);
	Py_DECREF(good);
	if (ok)
		return item;
	Py_DECREF(item);
	lz->stop = 1;
	return NULL;
}

static PyObject *
takewhile_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(takewhile_doc,
"takewhile(predicate, iterable) --> takewhile object\n\
\n\
Return successive entries from an iterable as long as the \n\
predicate evaluates to true for each entry.");

PyTypeObject takewhile_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.takewhile",		 /* tp_name */
	sizeof(takewhileobject),		 /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)takewhile_dealloc,	   /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	takewhile_doc,			   /* tp_doc */
	(traverseproc)takewhile_traverse,    /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)takewhile_getiter,	   /* tp_iter */
	(iternextfunc)takewhile_next,	   /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	takewhile_new,			 /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* islice object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *it;
	long	next;
	long	stop;
	long	step;
	long	cnt;
} isliceobject;

PyTypeObject islice_type;

static PyObject *
islice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *seq;
	long a1=0, a2=0, a3=0, start=0, stop=0, step=1;
	PyObject *it;
	int numargs;
	isliceobject *lz;

	numargs = PyTuple_Size(args);
	if (!PyArg_ParseTuple(args, "Ol|ll:islice", &seq, &a1, &a2, &a3))
		return NULL;

	if (numargs == 2) {
		stop = a1;
	} else if (numargs == 3) {
		start = a1;
		stop = a2;
	} else {
		start = a1;
		stop = a2;
		step = a3;
	}

	if (start<0 || stop<0) {
		PyErr_SetString(PyExc_ValueError,
		   "Indices for islice() must be positive.");
		return NULL;
	}

	if (step<1) {
		PyErr_SetString(PyExc_ValueError,
		   "Step must be one or larger for islice().");
		return NULL;
	}

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* create isliceobject structure */
	lz = (isliceobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(it);
		return NULL;
	}
	lz->it = it;
	lz->next = start;
	lz->stop = stop;
	lz->step = step;
	lz->cnt = 0L;

	return (PyObject *)lz;
}

static void
islice_dealloc(isliceobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->it);
	lz->ob_type->tp_free(lz);
}

static int
islice_traverse(isliceobject *lz, visitproc visit, void *arg)
{
	if (lz->it)
		return visit(lz->it, arg);
	return 0;
}

static PyObject *
islice_next(isliceobject *lz)
{
	PyObject *item;

	while (lz->cnt < lz->next) {
		item = PyIter_Next(lz->it);
		if (item == NULL)
			return NULL;
		Py_DECREF(item);
		lz->cnt++;
	}
	if (lz->cnt >= lz->stop)
		return NULL;
	item = PyIter_Next(lz->it);
	if (item == NULL)
		return NULL;
	lz->cnt++;
	lz->next += lz->step;
	return item;
}

static PyObject *
islice_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(islice_doc,
"islice(iterable, [start,] stop [, step]) --> islice object\n\
\n\
Return an iterator whose next() method returns selected values from an\n\
iterable.  If start is specified, will skip all preceding elements;\n\
otherwise, start defaults to zero.  Step defaults to one.  If\n\
specified as another value, step determines how many values are \n\
skipped between successive calls.  Works like a slice() on a list\n\
but returns an iterator.");

PyTypeObject islice_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.islice",		/* tp_name */
	sizeof(isliceobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)islice_dealloc,	  /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	islice_doc,			  /* tp_doc */
	(traverseproc)islice_traverse,	  /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)islice_getiter,	  /* tp_iter */
	(iternextfunc)islice_next,	  /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	islice_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* starmap object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *func;
	PyObject *it;
} starmapobject;

PyTypeObject starmap_type;

static PyObject *
starmap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *func, *seq;
	PyObject *it;
	starmapobject *lz;

	if (!PyArg_UnpackTuple(args, "starmap", 2, 2, &func, &seq))
		return NULL;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* create starmapobject structure */
	lz = (starmapobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(it);
		return NULL;
	}
	Py_INCREF(func);
	lz->func = func;
	lz->it = it;

	return (PyObject *)lz;
}

static void
starmap_dealloc(starmapobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->func);
	Py_XDECREF(lz->it);
	lz->ob_type->tp_free(lz);
}

static int
starmap_traverse(starmapobject *lz, visitproc visit, void *arg)
{
	int err;

	if (lz->it) {
		err = visit(lz->it, arg);
		if (err)
			return err;
	}
	if (lz->func) {
		err = visit(lz->func, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
starmap_next(starmapobject *lz)
{
	PyObject *args;
	PyObject *result;

	args = PyIter_Next(lz->it);
	if (args == NULL)
		return NULL;
	result = PyObject_Call(lz->func, args, NULL);
	Py_DECREF(args);
	return result;
}

static PyObject *
starmap_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(starmap_doc,
"starmap(function, sequence) --> starmap object\n\
\n\
Return an iterator whose values are returned from the function evaluated\n\
with a argument tuple taken from the given sequence.");

PyTypeObject starmap_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.starmap",		 /* tp_name */
	sizeof(starmapobject),		 /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)starmap_dealloc,	   /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	starmap_doc,			   /* tp_doc */
	(traverseproc)starmap_traverse,    /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)starmap_getiter,	   /* tp_iter */
	(iternextfunc)starmap_next,	   /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	starmap_new,			 /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* imap object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *iters;
	PyObject *argtuple;
	PyObject *func;
} imapobject;

PyTypeObject imap_type;

static PyObject *
imap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *it, *iters, *argtuple, *func;
	imapobject *lz;
	int numargs, i;

	numargs = PyTuple_Size(args);
	if (numargs < 2) {
		PyErr_SetString(PyExc_TypeError,
		   "imap() must have at least two arguments.");
		return NULL;
	}

	iters = PyTuple_New(numargs-1);
	if (iters == NULL)
		return NULL;

	argtuple = PyTuple_New(numargs-1);
	if (argtuple == NULL) {
		Py_DECREF(iters);
		return NULL;
	}

	for (i=1 ; i<numargs ; i++) {
		/* Get iterator. */
		it = PyObject_GetIter(PyTuple_GET_ITEM(args, i));
		if (it == NULL) {
			Py_DECREF(argtuple);
			Py_DECREF(iters);
			return NULL;
		}
		PyTuple_SET_ITEM(iters, i-1, it);
		Py_INCREF(Py_None);
		PyTuple_SET_ITEM(argtuple, i-1, Py_None);
	}

	/* create imapobject structure */
	lz = (imapobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(argtuple);
		Py_DECREF(iters);
		return NULL;
	}
	lz->iters = iters;
	lz->argtuple = argtuple;
	func = PyTuple_GET_ITEM(args, 0);
	Py_INCREF(func);
	lz->func = func;

	return (PyObject *)lz;
}

static void
imap_dealloc(imapobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->argtuple);
	Py_XDECREF(lz->iters);
	Py_XDECREF(lz->func);
	lz->ob_type->tp_free(lz);
}

static int
imap_traverse(imapobject *lz, visitproc visit, void *arg)
{
	int err;

	if (lz->iters) {
		err = visit(lz->iters, arg);
		if (err)
			return err;
	}
	if (lz->argtuple) {
		err = visit(lz->argtuple, arg);
		if (err)
			return err;
	}
	if (lz->func) {
		err = visit(lz->func, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
imap_next(imapobject *lz)
{
	PyObject *val;
	PyObject *argtuple=lz->argtuple;
	int numargs, i;

	numargs = PyTuple_Size(lz->iters);
	if (lz->func == Py_None) {
		argtuple = PyTuple_New(numargs);
		if (argtuple == NULL)
			return NULL;

		for (i=0 ; i<numargs ; i++) {
			val = PyIter_Next(PyTuple_GET_ITEM(lz->iters, i));
			if (val == NULL) {
				Py_DECREF(argtuple);
				return NULL;
			}
			PyTuple_SET_ITEM(argtuple, i, val);
		}
		return argtuple;
	} else {
		for (i=0 ; i<numargs ; i++) {
			val = PyIter_Next(PyTuple_GET_ITEM(lz->iters, i));
			if (val == NULL)
				return NULL;
			Py_DECREF(PyTuple_GET_ITEM(argtuple, i));
			PyTuple_SET_ITEM(argtuple, i, val);
		}
		return PyObject_Call(lz->func, argtuple, NULL);
	}
}

static PyObject *
imap_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(imap_doc,
"imap(func, *iterables) --> imap object\n\
\n\
Make an iterator that computes the function using arguments from\n\
each of the iterables.	Like map() except that it returns\n\
an iterator instead of a list and that it stops when the shortest\n\
iterable is exhausted instead of filling in None for shorter\n\
iterables.");

PyTypeObject imap_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.imap",	      /* tp_name */
	sizeof(imapobject),	      /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)imap_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	imap_doc,			/* tp_doc */
	(traverseproc)imap_traverse,	/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)imap_getiter,	/* tp_iter */
	(iternextfunc)imap_next,	/* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	imap_new,		      /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* times object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *obj;
	long	cnt;
} timesobject;

PyTypeObject times_type;

static PyObject *
times_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	timesobject *lz;
	PyObject *obj = Py_None;
	long cnt;

	if (!PyArg_ParseTuple(args, "l|O:times", &cnt, &obj))
		return NULL;

	if (cnt < 0) {
		PyErr_SetString(PyExc_ValueError,
		   "count for imap() cannot be negative.");
		return NULL;
	}

	/* create timesobject structure */
	lz = (timesobject *)type->tp_alloc(type, 0);
	if (lz == NULL)
		return NULL;
	lz->cnt = cnt;
	Py_INCREF(obj);
	lz->obj = obj;

	return (PyObject *)lz;
}

static void
times_dealloc(timesobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->obj);
	lz->ob_type->tp_free(lz);
}

static PyObject *
times_next(timesobject *lz)
{
	PyObject *obj = lz->obj;

	if (lz->cnt > 0) {
		lz->cnt--;
		Py_INCREF(obj);
		return obj;
	}
	return NULL;
}

static PyObject *
times_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(times_doc,
"times(n [,obj]) --> times object\n\
\n\
Return a times object whose .next() method returns n consecutive\n\
instances of obj (default is None).");

PyTypeObject times_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.times",	       /* tp_name */
	sizeof(timesobject),	       /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)times_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	times_doc,			 /* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)times_getiter,	 /* tp_iter */
	(iternextfunc)times_next,	 /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	times_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* ifilter object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *func;
	PyObject *it;
	long	 invert;
} ifilterobject;

PyTypeObject ifilter_type;

static PyObject *
ifilter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *func, *seq, *invert=NULL;
	PyObject *it;
	ifilterobject *lz;
	long inv=0;

	if (!PyArg_UnpackTuple(args, "ifilter", 2, 3, &func, &seq, &invert))
		return NULL;

	if (invert != NULL  &&	PyObject_IsTrue(invert))
		inv = 1;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* create ifilterobject structure */
	lz = (ifilterobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(it);
		return NULL;
	}
	Py_INCREF(func);
	lz->func = func;
	lz->it = it;
	lz->invert = inv;

	return (PyObject *)lz;
}

static void
ifilter_dealloc(ifilterobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->func);
	Py_XDECREF(lz->it);
	lz->ob_type->tp_free(lz);
}

static int
ifilter_traverse(ifilterobject *lz, visitproc visit, void *arg)
{
	int err;

	if (lz->it) {
		err = visit(lz->it, arg);
		if (err)
			return err;
	}
	if (lz->func) {
		err = visit(lz->func, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
ifilter_next(ifilterobject *lz)
{
	PyObject *item;
	long ok;

	for (;;) {
		item = PyIter_Next(lz->it);
		if (item == NULL)
			return NULL;

		if (lz->func == Py_None) {
			ok = PyObject_IsTrue(item);
		} else {
			PyObject *good;
			good = PyObject_CallFunctionObjArgs(lz->func,
							    item, NULL);
			if (good == NULL) {
				Py_DECREF(item);
				return NULL;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok ^ lz->invert)
			return item;
		Py_DECREF(item);
	}
}

static PyObject *
ifilter_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(ifilter_doc,
"ifilter(function or None, sequence [, invert]) --> ifilter object\n\
\n\
Return those items of sequence for which function(item) is true.  If\n\
invert is set to True, return items for which function(item) if False.\n\
If function is None, return the items that are true (unless invert is set).");

PyTypeObject ifilter_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.ifilter",		 /* tp_name */
	sizeof(ifilterobject),		 /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)ifilter_dealloc,	   /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	ifilter_doc,			   /* tp_doc */
	(traverseproc)ifilter_traverse,    /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)ifilter_getiter,	   /* tp_iter */
	(iternextfunc)ifilter_next,	   /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	ifilter_new,			 /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* count object ************************************************************/

typedef struct {
	PyObject_HEAD
	long	cnt;
} countobject;

PyTypeObject count_type;

static PyObject *
count_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	countobject *lz;
	long cnt = 0;

	if (!PyArg_ParseTuple(args, "|l:count", &cnt))
		return NULL;

	/* create countobject structure */
	lz = (countobject *)PyObject_New(countobject, &count_type);
	if (lz == NULL)
		return NULL;
	lz->cnt = cnt;

	return (PyObject *)lz;
}

static PyObject *
count_next(countobject *lz)
{
	return PyInt_FromLong(lz->cnt++);
}

static PyObject *
count_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(count_doc,
"count([firstval]) --> count object\n\
\n\
Return a count object whose .next() method returns consecutive\n\
integers starting from zero or, if specified, from firstval.");

PyTypeObject count_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.count",	       /* tp_name */
	sizeof(countobject),	       /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)PyObject_Del,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	count_doc,			 /* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)count_getiter,	 /* tp_iter */
	(iternextfunc)count_next,	 /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	count_new,			/* tp_new */
};


/* izip object ************************************************************/

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long	tuplesize;
	PyObject *ittuple;		/* tuple of iterators */
} izipobject;

PyTypeObject izip_type;

static PyObject *
izip_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	izipobject *lz;
	int i;
	PyObject *ittuple;  /* tuple of iterators */
	int tuplesize = PySequence_Length(args);

	if (tuplesize < 1) {
		PyErr_SetString(PyExc_TypeError,
				"izip() requires at least one sequence");
		return NULL;
	}

	/* args must be a tuple */
	assert(PyTuple_Check(args));

	/* obtain iterators */
	ittuple = PyTuple_New(tuplesize);
	if(ittuple == NULL)
		return NULL;
	for (i=0; i < tuplesize; ++i) {
		PyObject *item = PyTuple_GET_ITEM(args, i);
		PyObject *it = PyObject_GetIter(item);
		if (it == NULL) {
			if (PyErr_ExceptionMatches(PyExc_TypeError))
				PyErr_Format(PyExc_TypeError,
				    "izip argument #%d must support iteration",
				    i+1);
			Py_DECREF(ittuple);
			return NULL;
		}
		PyTuple_SET_ITEM(ittuple, i, it);
	}

	/* create izipobject structure */
	lz = (izipobject *)type->tp_alloc(type, 0);
	if (lz == NULL) {
		Py_DECREF(ittuple);
		return NULL;
	}
	lz->ittuple = ittuple;
	lz->tuplesize = tuplesize;

	return (PyObject *)lz;
}

static void
izip_dealloc(izipobject *lz)
{
	PyObject_GC_UnTrack(lz);
	Py_XDECREF(lz->ittuple);
	lz->ob_type->tp_free(lz);
}

static int
izip_traverse(izipobject *lz, visitproc visit, void *arg)
{
	if (lz->ittuple)
		return visit(lz->ittuple, arg);
	return 0;
}

static PyObject *
izip_next(izipobject *lz)
{
	int i;
	long tuplesize = lz->tuplesize;
	PyObject *result;
	PyObject *it;
	PyObject *item;

	result = PyTuple_New(tuplesize);
	if (result == NULL)
		return NULL;

	for (i=0 ; i < tuplesize ; i++) {
		it = PyTuple_GET_ITEM(lz->ittuple, i);
		item = PyIter_Next(it);
		if (item == NULL) {
			Py_DECREF(result);
			return NULL;
		}
		PyTuple_SET_ITEM(result, i, item);
	}
	return result;
}

static PyObject *
izip_getiter(PyObject *lz)
{
	Py_INCREF(lz);
	return lz;
}

PyDoc_STRVAR(izip_doc,
"izip(iter1 [,iter2 [...]]) --> izip object\n\
\n\
Return a izip object whose .next() method returns a tuple where\n\
the i-th element comes from the i-th iterable argument.  The .next()\n\
method continues until the shortest iterable in the argument sequence\n\
is exhausted and then it raises StopIteration.  Works like the zip()\n\
function but consumes less memory by returning an iterator instead of\n\
a list.");

PyTypeObject izip_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.izip",		 /* tp_name */
	sizeof(izipobject),		 /* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)izip_dealloc,	   /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	izip_doc,			   /* tp_doc */
	(traverseproc)izip_traverse,    /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)izip_getiter,	   /* tp_iter */
	(iternextfunc)izip_next,	   /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	izip_new,			 /* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* repeat object ************************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *element;
} repeatobject;

PyTypeObject repeat_type;

static PyObject *
repeat_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	repeatobject *ro;
	PyObject *element;

	if (!PyArg_UnpackTuple(args, "repeat", 1, 1, &element))
		return NULL;

	ro = (repeatobject *)type->tp_alloc(type, 0);
	if (ro == NULL)
		return NULL;
	Py_INCREF(element);
	ro->element = element;
	return (PyObject *)ro;
}

static void
repeat_dealloc(repeatobject *ro)
{
	PyObject_GC_UnTrack(ro);
	Py_XDECREF(ro->element);
	ro->ob_type->tp_free(ro);
}

static int
repeat_traverse(repeatobject *ro, visitproc visit, void *arg)
{
	if (ro->element)
		return visit(ro->element, arg);
	return 0;
}

static PyObject *
repeat_next(repeatobject *ro)
{
	Py_INCREF(ro->element);
	return ro->element;
}

static PyObject *
repeat_getiter(PyObject *ro)
{
	Py_INCREF(ro);
	return ro;
}

PyDoc_STRVAR(repeat_doc,
"repeat(element) -> create an iterator which returns the element endlessly.");

PyTypeObject repeat_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"itertools.repeat",		/* tp_name */
	sizeof(repeatobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)repeat_dealloc,	  /* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	repeat_doc,			  /* tp_doc */
	(traverseproc)repeat_traverse,	  /* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)repeat_getiter,	  /* tp_iter */
	(iternextfunc)repeat_next,	  /* tp_iternext */
	0,				/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	repeat_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"Functional tools for creating and using iterators.\n\
\n\
Infinite iterators:\n\
count([n]) --> n, n+1, n+2, ...\n\
repeat(elem) --> elem, elem, elem, ...\n\
\n\
Iterators terminating on the shortest input sequence:\n\
izip(p, q, ...) --> (p[0], q[0]), (p[1], q[1]), ... \n\
ifilter(pred, seq, invert=False) --> elements of seq where\n\
       pred(elem) is True (or False if invert is set)\n\
islice(seq, [start,] stop [, step]) --> elements from\n\
       seq[start:stop:step]\n\
imap(fun, p, q, ...) --> fun(p0, q0), fun(p1, q1), ...\n\
starmap(fun, seq) --> fun(*seq[0]), fun(*seq[1]), ...\n\
times(n, [obj]) --> obj, obj, ... for n times.  obj defaults to None\n\
takewhile(pred, seq) --> seq[0], seq[1], until pred fails\n\
dropwhile(pred, seq) --> seq[n], seq[n+1], starting when pred fails\n\
");


PyMODINIT_FUNC
inititertools(void)
{
	PyObject *m;
	m = Py_InitModule3("itertools", NULL, module_doc);

	PyModule_AddObject(m, "dropwhile", (PyObject *)&dropwhile_type);
	if (PyType_Ready(&dropwhile_type) < 0)
		return;
	Py_INCREF(&dropwhile_type);

	PyModule_AddObject(m, "takewhile", (PyObject *)&takewhile_type);
	if (PyType_Ready(&takewhile_type) < 0)
		return;
	Py_INCREF(&takewhile_type);

	PyModule_AddObject(m, "islice", (PyObject *)&islice_type);
	if (PyType_Ready(&islice_type) < 0)
		return;
	Py_INCREF(&islice_type);

	PyModule_AddObject(m, "starmap", (PyObject *)&starmap_type);
	if (PyType_Ready(&starmap_type) < 0)
		return;
	Py_INCREF(&starmap_type);

	PyModule_AddObject(m, "imap", (PyObject *)&imap_type);
	if (PyType_Ready(&imap_type) < 0)
		return;
	Py_INCREF(&imap_type);

	PyModule_AddObject(m, "times", (PyObject *)&times_type);
	if (PyType_Ready(&times_type) < 0)
		return;
	Py_INCREF(&times_type);

	if (PyType_Ready(&ifilter_type) < 0)
		return;
	Py_INCREF(&ifilter_type);
	PyModule_AddObject(m, "ifilter", (PyObject *)&ifilter_type);

	if (PyType_Ready(&count_type) < 0)
		return;
	Py_INCREF(&count_type);
	PyModule_AddObject(m, "count", (PyObject *)&count_type);

	if (PyType_Ready(&izip_type) < 0)
		return;
	Py_INCREF(&izip_type);
	PyModule_AddObject(m, "izip", (PyObject *)&izip_type);

	if (PyType_Ready(&repeat_type) < 0)
		return;
	Py_INCREF(&repeat_type);
	PyModule_AddObject(m, "repeat", (PyObject *)&repeat_type);
}
