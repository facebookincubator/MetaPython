# Copyright (c) Meta, Inc. and its affiliates. All Rights Reserved
# File added for Lazy Imports

import self
import importlib

original = importlib.set_lazy_imports(
    True,
    excluding=["test.lazyimports.data.excluding.bar"],
    eager={"sentinel.preserved"},
)
try:
    previous = importlib.set_lazy_imports(
        excluding=["test.lazyimports.data.excluding.foo"]
    )
    self.assertEqual(previous[1], ["test.lazyimports.data.excluding.bar"])
    self.assertEqual(previous[2], {"sentinel.preserved"})

    current = importlib.set_lazy_imports(True)
    self.assertEqual(current[1], ["test.lazyimports.data.excluding.foo"])
    self.assertEqual(current[2], {"sentinel.preserved"})
finally:
    importlib.set_lazy_imports(*original)
