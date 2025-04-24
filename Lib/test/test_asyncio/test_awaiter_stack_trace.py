import asyncio
import unittest

class GetAsyncStackTests(unittest.TestCase):
    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(None)

    def tearDown(self):
        self.loop.close()
        self.loop = None
        asyncio.set_event_loop_policy(None)

    def check_stack(self, frames, expected_funcs):
        frames.reverse()
        given = [f.f_code.co_qualname for f in frames[-len(expected_funcs):]]
        expected = [f.__code__.co_qualname for f in expected_funcs]
        self.assertEqual(given, expected)

    def test_single_task(self):
        async def coro():
            await coro2()

        async def coro2():
            stack = asyncio.ci_get_async_stack()
            self.check_stack(stack, [coro, coro2])

        self.loop.run_until_complete(coro())

    def test_cross_tasks(self):
        async def coro():
            t = asyncio.ensure_future(coro2())
            await t

        async def coro2():
            t = asyncio.ensure_future(coro3())
            await t

        async def coro3():
            stack = asyncio.ci_get_async_stack()
            self.check_stack(stack, [coro, coro2, coro3])

        self.loop.run_until_complete(coro())

    def test_cross_gather(self):
        async def coro():
            await asyncio.gather(coro2(), coro2())

        async def coro2():
            stack = asyncio.ci_get_async_stack()
            self.check_stack(stack, [coro, coro2])

        self.loop.run_until_complete(coro())


if __name__ == '__main__':
    unittest.main()
