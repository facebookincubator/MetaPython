
#define _PY_INTERPRETER

#include "Python.h"
#include "frameobject.h"
#include "pycore_code.h"          // stats
#include "pycore_frame.h"
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "opcode.h"

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    Py_VISIT(frame->frame_obj);
    Py_VISIT(frame->f_locals);
    Py_VISIT(frame->f_funcobj);
    Py_VISIT(frame->f_code);
   /* locals */
    PyObject **locals = _PyFrame_GetLocalsArray(frame);
    int i = 0;
    /* locals and stack */
    for (; i <frame->stacktop; i++) {
        Py_VISIT(locals[i]);
    }
    return 0;
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame)
{
    PyObject *exc = PyErr_GetRaisedException();

    if (!PyFunction_Check(frame->f_funcobj)) {
        if (_PyFrame_EnsureFrameFullyInitialized(frame) < 0) {
            return NULL;
        }

        PyFrameObject *res =  frame->frame_obj;
        if (res != NULL) {
            PyErr_SetRaisedException(exc);
            return res;
        }
    }

    assert(frame->frame_obj == NULL);

    PyFrameObject *f = _PyFrame_New_NoTrack(frame->f_code);
    if (f == NULL) {
        Py_XDECREF(exc);
        return NULL;
    }
    PyErr_SetRaisedException(exc);
    if (frame->frame_obj) {
        // GH-97002: How did we get into this horrible situation? Most likely,
        // allocating f triggered a GC collection, which ran some code that
        // *also* created the same frame... while we were in the middle of
        // creating it! See test_sneaky_frame_object in test_frame.py for a
        // concrete example.
        //
        // Regardless, just throw f away and use that frame instead, since it's
        // already been exposed to user code. It's actually a bit tricky to do
        // this, since we aren't backed by a real _PyInterpreterFrame anymore.
        // Just pretend that we have an owned, cleared frame so frame_dealloc
        // doesn't make the situation worse:
        f->f_frame = (_PyInterpreterFrame *)f->_f_frame_data;
        f->f_frame->owner = FRAME_CLEARED;
        f->f_frame->frame_obj = f;
        Py_DECREF(f);
        return frame->frame_obj;
    }
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    assert(frame->owner != FRAME_CLEARED);
    f->f_frame = frame;
    frame->frame_obj = f;
    return f;
}

void
_PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    assert(src->stacktop >= src->f_code->co_nlocalsplus);
    Py_ssize_t size = ((char*)&src->localsplus[src->stacktop]) - (char *)src;
    memcpy(dest, src, size);
    // Don't leave a dangling pointer to the old frame when creating generators
    // and coroutines:
    dest->previous = NULL;
}


static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame)
{
    assert(frame->owner != FRAME_OWNED_BY_CSTACK);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    assert(frame->owner != FRAME_CLEARED);
    Py_ssize_t size = ((char*)&frame->localsplus[frame->stacktop]) - (char *)frame;
    Py_INCREF(frame->f_code);
    memcpy((_PyInterpreterFrame *)f->_f_frame_data, frame, size);
    frame = (_PyInterpreterFrame *)f->_f_frame_data;
    f->f_frame = frame;
    frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    if (_PyFrame_IsIncomplete(frame)) {
        // This may be a newly-created generator or coroutine frame. Since it's
        // dead anyways, just pretend that the first RESUME ran:
        PyCodeObject *code = frame->f_code;
        frame->prev_instr = _PyCode_CODE(code) + code->_co_firsttraceable;
    }
    assert(!_PyFrame_IsIncomplete(frame));
    assert(f->f_back == NULL);
    _PyInterpreterFrame *prev = _PyFrame_GetFirstComplete(frame->previous);
    frame->previous = NULL;
    if (prev) {
        assert(prev->owner != FRAME_OWNED_BY_CSTACK);
        /* Link PyFrameObjects.f_back and remove link through _PyInterpreterFrame.previous */
        PyFrameObject *back = _PyFrame_GetFrameObject(prev);
        if (back == NULL) {
            /* Memory error here. */
            assert(PyErr_ExceptionMatches(PyExc_MemoryError));
            /* Nothing we can do about it */
            PyErr_Clear();
        }
        else {
            f->f_back = (PyFrameObject *)Py_NewRef(back);
        }
    }
    if (!_PyObject_GC_IS_TRACKED((PyObject *)f)) {
        _PyObject_GC_TRACK((PyObject *)f);
    }
}

void
_PyFrame_ClearExceptCode(_PyInterpreterFrame *frame)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the enclosing generator, if any. */
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
        _PyFrame_GetGenerator(frame)->gi_frame_state == FRAME_CLEARED);
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->cframe->current_frame != frame);
    if (frame->frame_obj) {
        PyFrameObject *f = frame->frame_obj;
        frame->frame_obj = NULL;
        if (Py_REFCNT(f) > 1) {
            take_ownership(f, frame);
            Py_DECREF(f);
            return;
        }
        Py_DECREF(f);
    }
    assert(frame->stacktop >= 0);
    for (int i = 0; i < frame->stacktop; i++) {
        Py_XDECREF(frame->localsplus[i]);
    }
    Py_XDECREF(frame->frame_obj);
    Py_XDECREF(frame->f_locals);
    Py_DECREF(frame->f_funcobj);
}

// Calls the frame reifier and returns the original function object
PyFunctionObject *
_PyFrame_ReifyFrame(_PyInterpreterFrame *frame)
{
    // Profilers can reach this from a signal handler on a thread that has
    // released the GIL, where there's no thread state to reify with.
    if (_PyThreadState_GET() == NULL) {
        return NULL;
    }

    PyObject *exc = PyErr_GetRaisedException();

     // Tracing this allocation can throw off tracemalloc expectations
    // so we temporarily disable.
    int cur_tracemalloc = _PyRuntime.tracemalloc.config.tracing;
    _PyRuntime.tracemalloc.config.tracing = 0;
    PyObject *frame_addr = PyLong_FromVoidPtr((void*)frame);
    if (frame_addr == NULL) {
        goto error;
    }
    if (PyFunction_Check(frame->f_funcobj)) {
        // re-entrancy during the allocation caused the frame to be initialized
        PyErr_SetRaisedException(exc);
        return 0;
    }
    PyObject *tmp = PyObject_Vectorcall(frame->f_funcobj, &frame_addr, 1, NULL);
    Py_DECREF(frame_addr);
    if (tmp == NULL) {
        goto error;
    }

    if (!PyFunction_Check(tmp)) {
        PyErr_SetString(PyExc_RuntimeError,
            "expected frame handler to return original function");
        Py_DECREF(tmp);
        goto error;
    }
    Py_DECREF(tmp); // the function is kept alive by the frame
    PyErr_SetRaisedException(exc);
error:
    _PyRuntime.tracemalloc.config.tracing = cur_tracemalloc;

    return (PyFunctionObject *)tmp;
}

PyObject *
_PyFrame_GetModule(_PyInterpreterFrame *frame) {
    PyFunctionObject *func = _PyFrame_GetFunction(frame);
    if (func == NULL) {
        return NULL;
    }
    return PyFunction_GetModule((PyObject*)func);
}

int
_PyFrame_InitializeExternalFrame(_PyInterpreterFrame *frame) {
    return _PyFrame_ReifyFrame(frame) != NULL;
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame)
{
    PyObject *code = (PyObject *)frame->f_code;
    Py_INCREF(code);
    return code;
}

int
PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame)
{
    return _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
}

int
PyUnstable_InterpreterFrame_GetLine(_PyInterpreterFrame *frame)
{
    _PyFrame_EnsureFrameFullyInitialized(frame);
    int addr = _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
    return PyCode_Addr2Line(frame->f_code, addr);
}
