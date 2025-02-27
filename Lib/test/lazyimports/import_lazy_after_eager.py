import importlib
importlib.set_lazy_imports(eager=["test.lazyimports.data.module_same_name_aliased", "test.lazyimports.data.module_same_name_aliased.foo.foo"])

from test.lazyimports.data.module_same_name_aliased import foo
if foo != 42:
    raise ValueError()

importlib.set_lazy_imports(eager=[])

from test.lazyimports.data.module_same_name_aliased.foo import foo
if foo != 42:
    raise ValueError()


from test.lazyimports.data.module_same_name_aliased import foo
if foo != 42:
    raise ValueError()
