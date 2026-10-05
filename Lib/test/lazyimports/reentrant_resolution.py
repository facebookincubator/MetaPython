# Copyright (c) Meta, Inc. and its affiliates. All Rights Reserved
# File added for Lazy Imports

"""
Re-entering a lazy import's resolution on the same thread (here from a weakref
callback fired mid-resolution) must resolve it, not raise ImportCycleError, when
the target module is already fully imported.
"""
import self
import builtins
import importlib
import weakref

importlib.import_module("json")
from json import dumps


class Obj:
    pass


obj = Obj()
seen = []


def callback(_):
    try:
        seen.append(dumps)
    except ImportError as e:
        seen.append(e)


ref = weakref.ref(obj, callback)
real_import = builtins.__import__


def hook(*args, **kwargs):
    global obj
    obj = None
    return real_import(*args, **kwargs)


builtins.__import__ = hook
try:
    resolved = dumps
finally:
    builtins.__import__ = real_import

if importlib.is_lazy_imports_enabled():
    self.assertEqual(seen, [resolved])
else:
    self.assertEqual(seen, [])
