# Copyright (c) Meta, Inc. and its affiliates. All Rights Reserved
# File added for Lazy Imports

import self
if self._lazy_imports:
    self.skipTest("Test relevant only when running with global lazy imports disabled")

import importlib

importlib.set_lazy_imports(eager=[
    "test.lazyimports.data.metasyntactic.foo.bar",
    "test.lazyimports.data.metasyntactic.plugh.Plugh",
])

import test.lazyimports.data.metasyntactic.foo as foo
self.assertTrue(importlib.is_lazy_import(globals(), "foo"))  # should be lazy

from test.lazyimports.data.metasyntactic.foo import bar
self.assertFalse(importlib.is_lazy_import(globals(), "bar"))  # listed in the eager list, so should not be lazy

from test.lazyimports.data.metasyntactic.foo.bar import Bar
self.assertTrue(importlib.is_lazy_import(globals(), "Bar"))  # should be lazy

import test.lazyimports.data.metasyntactic.waldo.fred as fred
self.assertTrue(importlib.is_lazy_import(globals(), "fred"))  # this should be lazy

from test.lazyimports.data.metasyntactic.waldo.fred import Fred
self.assertTrue(importlib.is_lazy_import(globals(), "Fred"))  # this should be lazy

from test.lazyimports.data.metasyntactic.waldo import fred
self.assertTrue(importlib.is_lazy_import(globals(), "fred"))  # this should be lazy

import test.lazyimports.data.metasyntactic.plugh as plugh
self.assertTrue(importlib.is_lazy_import(globals(), "plugh"))  # this should be lazy

from test.lazyimports.data.metasyntactic.plugh import Plugh
self.assertFalse(importlib.is_lazy_import(globals(), "Plugh"))  # explicitly eager
