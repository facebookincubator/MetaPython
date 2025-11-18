#ifndef Py_CPYTHON_IMPORT_H
#  error "this header file must not be included directly"
#endif

#ifdef ENABLE_LAZY_IMPORTS
PyAPI_FUNC(PyObject *) _PyImport_GetModule(PyThreadState *tstate, PyObject *name);
PyAPI_FUNC(PyObject *) PyImport_SetLazyImports(
    PyObject *enabled, PyObject *excluding, PyObject *eager);
PyAPI_FUNC(PyObject *) _PyImport_SetLazyImportsInModule(
    PyObject *enabled);
PyAPI_FUNC(int) _PyImport_IsLazyImportsActive(PyThreadState *tstate);
PyAPI_FUNC(int) PyImport_IsLazyImportsEnabled(void);
#endif

struct _inittab {
    const char *name;           /* ASCII encoded string */
    PyObject* (*initfunc)(void);
};
// This is not used after Py_Initialize() is called.
PyAPI_DATA(struct _inittab *) PyImport_Inittab;
PyAPI_FUNC(int) PyImport_ExtendInittab(struct _inittab *newtab);

struct _frozen {
    const char *name;                 /* ASCII encoded string */
    const unsigned char *code;
    int size;
    int is_package;
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

PyAPI_DATA(const struct _frozen *) PyImport_FrozenModules;

PyAPI_FUNC(PyObject*) PyImport_ImportModuleAttr(
    PyObject *mod_name,
    PyObject *attr_name);
PyAPI_FUNC(PyObject*) PyImport_ImportModuleAttrString(
    const char *mod_name,
    const char *attr_name);

    // Custom importers may use this API to initialize statically linked
// extension modules directly from a spec and init function,
// without needing to go through inittab
PyAPI_FUNC(PyObject *) PyImport_CreateModuleFromInitfunc(
    PyObject *spec,
    PyObject *(*initfunc)(void));

// START META PATCH (expose C API to call a module init function for statically linked extensions)
// Custom importers may use this API to initialize statically linked
// extension modules directly from a spec and init function,
// without needing to go through inittab
// TODO: migrate to the upstream version, PyImport_CreateModuleFromInitfunc, and delete this
PyAPI_FUNC(PyObject *)
_Ci_PyImport_CreateBuiltinFromSpecAndInitfunc(
    PyObject *spec,
    PyObject* (*initfunc)(void)
    );
// END META PATCH
