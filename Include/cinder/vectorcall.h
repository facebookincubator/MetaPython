#pragma once

#define Ci_Py_AWAITED_CALL_MARKER  ((size_t)1 << (8 * sizeof(size_t) - 2))
#define Ci_Py_AWAITED_CALL(n) ((n)&Ci_Py_AWAITED_CALL_MARKER)
#define Ci_Py_VECTORCALL_ARGUMENT_MASK    Ci_Py_AWAITED_CALL_MARKER

/* Same as PyVectorcall_Call but allows passing extra flags to function being called */
CiAPI_FUNC(PyObject *) Ci_PyVectorcall_Call_WithFlags(
    PyObject *callable, PyObject *tuple, PyObject *kwargs, size_t flags);
