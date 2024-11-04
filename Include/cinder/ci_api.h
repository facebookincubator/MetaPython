#pragma once

/*
 * Macros for marking components in core CPython we currently export for the
 * CinderX module. This includes things added by Cinder and things which
 * already existed but which weren't public.
 *
 * The intent is grepping for "CiAPI" reveals everything the CinderX module may
 * depend on in the core CPython code. Eliminating all of these is one of the
 * prerequisites for CinderX being compatible with non-Cinder Python.
 */

#include "exports.h"

// These function the same as PyAPI_* - exporting symbols for use in .so's etc.
#define CiAPI_FUNC(RTYPE) Py_EXPORTED_SYMBOL RTYPE
#ifdef __clang__
#  ifdef __cplusplus
#    define CiAPI_DATA(RTYPE) Py_EXPORTED_SYMBOL extern "C" RTYPE
#  else
#    define CiAPI_DATA(RTYPE) Py_EXPORTED_SYMBOL extern RTYPE
#  endif
#else
#  ifdef __cplusplus
#    define CiAPI_DATA(RTYPE) extern "C" Py_EXPORTED_SYMBOL RTYPE
#  else
#    define CiAPI_DATA(RTYPE) extern Py_EXPORTED_SYMBOL RTYPE
#  endif
#endif

// Clang seems to (always?) make symbols for static inline functions.
#ifdef __clang__
#  define CiAPI_STATIC_INLINE_FUNC(RTYPE) static inline Py_EXPORTED_SYMBOL RTYPE
#else
#  define CiAPI_STATIC_INLINE_FUNC(RTYPE) static inline RTYPE
#endif
