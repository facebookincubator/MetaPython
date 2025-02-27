import _imp
import importlib
import os
import sys
import unittest


class LazyImportsTest(unittest.TestCase):
    pass

def make_test(test_name: str, lazy_imports: bool) -> object:
    def test_case(self):
        original_lazy_modules = sys.lazy_modules.copy()
        original_modules = sys.modules.copy()
        try:
            sys.lazy_modules.clear()
            sys.modules["self"] = self
            for modname in list(sys.modules):
                if modname == "test" or modname.startswith("test."):
                    del sys.modules[modname]
            stripped_modules = sys.modules.copy()
            msg = f"{test_name}{' (lazy)' if lazy_imports else ' (eager)'}"
            self._test_name = test_name
            self._lazy_imports = lazy_imports
            previously = _imp._set_lazy_imports(lazy_imports)
            try:
                importlib.import_module(test_name)
            finally:
                _imp._set_lazy_imports(*previously)
                del self._test_name
                del self._lazy_imports
        finally:
            sys.lazy_modules.clear()
            sys.lazy_modules.update(original_lazy_modules)
            sys.modules.clear()
            sys.modules.update(original_modules)

    return test_case

base = os.path.dirname(__file__)
for path in os.listdir(os.path.join(base, "lazyimports")):
    if path == "data" or path.startswith(("_", ".")):
        continue
    path = path.removesuffix(".py")
    for lazy_imports in (True, False):
        test_name = "test_" + path.replace(os.sep, "_") + ("_lazy" if lazy_imports else "_eager")
        test_mod = "test.lazyimports." + path.replace(os.sep, ".")
        setattr(LazyImportsTest, test_name, make_test(test_mod, lazy_imports))
