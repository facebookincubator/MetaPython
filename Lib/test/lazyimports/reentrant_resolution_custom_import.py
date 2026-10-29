# Copyright (c) Meta, Inc. and its affiliates. All Rights Reserved
# File added for Lazy Imports

"""
Re-entering a lazy import created under a custom __import__ (as torch.package's
PackageImporter installs in a private builtins dict) must not resolve it from sys.modules: the custom
__import__ may serve a different module under the same name.
"""
import self
import sys
import types
import weakref

shared = types.ModuleType("reentrant_custom_target")
shared.value = "from sys.modules"

private = types.ModuleType("reentrant_custom_target")
private.value = "from custom __import__"


class Obj:
    pass


state = {"obj": Obj(), "armed": False}


def custom_import(*args, **kwargs):
    if state["armed"]:
        state["obj"] = None
    return private


ns = {"__builtins__": {"__import__": custom_import, "ImportError": ImportError}}
source = (
    "from reentrant_custom_target import value\n"
    "seen = []\n"
    "def callback(_):\n"
    "    try:\n"
    "        seen.append(value)\n"
    "    except ImportError as e:\n"
    "        seen.append(e)\n"
)
# exec() of a str disables lazy imports; torch.package compiles first.
exec(compile(source, "<reentrant_custom_target>", "exec"), ns)
sys.modules[shared.__name__] = shared
ref = weakref.ref(state["obj"], ns["callback"])
state["armed"] = True
resolved = eval("value", ns)

self.assertEqual(resolved, "from custom __import__")
self.assertNotIn("from sys.modules", ns["seen"])
