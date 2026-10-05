import threading
import unittest

from test.support import import_helper
from test.support import threading_helper
# Raise SkipTest if subinterpreters not supported.
_interpreters = import_helper.import_module('_interpreters')


class GCImplRegistryTests(unittest.TestCase):

    @threading_helper.requires_working_threading()
    def test_concurrent_create_destroy(self):
        # Every interpreter registers its GC state in a process-wide list on
        # creation and unlinks it on destruction. Isolated interpreters have
        # their own GIL, so those updates race unless the list is locked.
        def task():
            for _ in range(100):
                interp = _interpreters.create('isolated')
                _interpreters.exec(interp, 'import gc; gc.collect()')
                _interpreters.destroy(interp)

        threads = [threading.Thread(target=task) for _ in range(16)]
        with threading_helper.start_threads(threads):
            pass


if __name__ == "__main__":
    unittest.main()
