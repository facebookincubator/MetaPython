import sys
import threading
import unittest

from test import support
from test.support import threading_helper


class Holder:
    pass


def make_shared(func, *args):
    t = threading.Thread(target=func, args=args)
    t.start()
    t.join()


@threading_helper.requires_working_threading()
class TestDelayedFreeReentrancy(unittest.TestCase):
    ROUNDS = 1000

    def queue_finalizers(self, callback, count=ROUNDS):
        class Finalizer:
            def __del__(self):
                callback()

        holders = [Holder() for _ in range(count)]
        for h in holders:
            h.__dict__ = {"f": Finalizer()}
        return holders

    def test_dict_resize(self):
        d = {}
        make_shared(d.get, 0)
        holders = self.queue_finalizers(lambda: d.__setitem__(object(), 0),
                                        count=100)
        for h in holders:
            h.__dict__ = {}
        for _ in range(self.ROUNDS):
            for k in list(d):
                del d[k]
            for i in range(6):
                d[i] = i
            self.assertEqual(len(d), sum(1 for _ in d))

    def test_list_resize(self):
        L = []
        make_shared(L.append, 0)
        holders = self.queue_finalizers(L.clear)
        for n, h in enumerate(holders):
            h.__dict__ = {}
            if n % 2:
                h.__dict__ = {}
            L.clear()
            for i in range(9):
                L.insert(len(L), i)
            self.assertEqual(len(L), len(list(L)))

    def test_type_cache_invalidation(self):
        class C:
            x = None

        holders = self.queue_finalizers(lambda: C.x)
        for h in holders:
            h.__dict__ = {}
            C.x
            new = object()
            C.x = new
            self.assertIs(C.x, new)


    @unittest.skipIf(sys._is_gil_enabled(), "requires free-threaded Python")
    def test_deferred_finalizers_wake_evaluator(self):
        with support.disable_gc():
            finalized = []
            holders = self.queue_finalizers(lambda: finalized.append(None))
            for h in holders:
                h.__dict__ = {}
            self.assertTrue(finalized)

    @unittest.skipIf(sys._is_gil_enabled(), "requires free-threaded Python")
    def test_finalizer_requeues_deferred_work(self):
        with support.disable_gc():
            finalized = []
            nested = self.queue_finalizers(lambda: finalized.append(None))

            def requeue():
                for h in nested:
                    h.__dict__ = {}
                nested.clear()

            holders = self.queue_finalizers(requeue)
            for h in holders:
                h.__dict__ = {}
            self.assertTrue(finalized)

    @unittest.skipIf(sys._is_gil_enabled(), "requires free-threaded Python")
    def test_refcount_merge_and_deferred_finalizers(self):
        with support.disable_gc():
            merged = []

            class Finalizer:
                def __del__(self):
                    merged.append(None)

            objects = [Finalizer() for _ in range(self.ROUNDS)]
            finalized = []
            holders = self.queue_finalizers(lambda: finalized.append(None))
            worker = threading.Thread(target=objects.clear)
            with threading_helper.start_threads([worker]):
                for h in holders:
                    h.__dict__ = {}
            self.assertEqual(len(merged), self.ROUNDS)
            self.assertTrue(finalized)


if __name__ == "__main__":
    unittest.main()
