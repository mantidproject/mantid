#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import

# system imports
import unittest

# 3rdparty imports

# local imports
from mantidqt.utils.async import AsyncTask, blocking_async_task


class AsyncTaskTest(unittest.TestCase):

    class Receiver(object):
        success_cb_called, error_cb_called, finished_cb_called = False, False, False
        task_output = None,
        task_exc_type, task_exc, task_exc_stack = None, None, None

        def on_success(self, task_result):
            self.success_cb_called = True
            self.task_output = task_result.output

        def on_error(self, task_result):
            self.error_cb_called = True
            self.task_exc_type = task_result.exc_type
            self.task_exc = task_result.exc_value
            self.task_exc_stack = task_result.stack

        def on_finished(self):
            self.finished_cb_called = True

    # ---------------------------------------------------------------
    # Success cases
    # ---------------------------------------------------------------
    def test_successful_no_arg_operation_calls_success_and_finished_callback(self):
        def foo():
            return 42

        recv = AsyncTaskTest.Receiver()
        t = AsyncTask(foo, success_cb=recv.on_success, error_cb=recv.on_error,
                      finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)
        self.assertEqual(42, recv.task_output)

    def test_successful_positional_args_operation_calls_success_and_finished_callback(self):
        def foo(shift):
            return 42 + shift

        recv = AsyncTaskTest.Receiver()
        shift = 2
        t = AsyncTask(foo, args = (shift,),
                      success_cb=recv.on_success, error_cb=recv.on_error,
                      finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)
        self.assertEqual(42 + shift, recv.task_output)

    def test_successful_args_and_kwargs_operation_calls_success_and_finished_callback(self):
        def foo(scale, shift):
            return scale*42 + shift

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(foo, args = (scale,), kwargs={'shift': shift},
                      success_cb=recv.on_success, error_cb=recv.on_error,
                      finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)
        self.assertEqual(scale*42 + shift, recv.task_output)

    def test_unsuccessful_no_arg_operation_calls_error_and_finished_callback(self):
        def foo():
            # this is a bad operation
            # that should appear in the stack trace
            raise RuntimeError("Bad operation")

        recv = AsyncTaskTest.Receiver()
        t = AsyncTask(foo, success_cb=recv.on_success,
                      error_cb=recv.on_error,
                      finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError),
                        msg="Expected RuntimeError, found " + recv.task_exc.__class__.__name__)
        self.assertEqual(2, len(recv.task_exc_stack))
        # line number of self.target in async.py
        self.assertEqual(100, recv.task_exc_stack[0][1])
        # line number of raise statement above
        self.assertEqual(101, recv.task_exc_stack[1][1])

    def test_unsuccessful_args_and_kwargs_operation_calls_error_and_finished_callback(self):
        def foo(scale, shift):
            raise RuntimeError("Bad operation")

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(foo, args = (scale,), kwargs={'shift': shift},
                      success_cb=recv.on_success, error_cb=recv.on_error,
                      finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError))

    def test_unsuccessful_operation_with_error_cb_and_stack_chop(self):
        def foo(scale, shift):
            def bar():
                raise RuntimeError("Bad operation")
            bar()

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(foo, args = (scale,), kwargs={'shift': shift}, stack_chop=1,
                      error_cb=recv.on_error)
        t.start()
        t.join()
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError))
        self.assertEqual(2, len(recv.task_exc_stack))
        self.assertEqual(140, recv.task_exc_stack[0][1])
        self.assertEqual(139, recv.task_exc_stack[1][1])

    # ---------------------------------------------------------------
    # Failure cases
    # ---------------------------------------------------------------
    def test_non_callable_object_raises_exception(self):
        self.assertRaises(TypeError, AsyncTask, None)
        self.assertRaises(TypeError, AsyncTask, object())


class BlockingAsyncTaskTest(unittest.TestCase):

    # ---------------------------------------------------------------
    # Success cases
    # ---------------------------------------------------------------
    def test_successful_no_arg_operation_without_callback(self):
        def foo():
            return 42

        self.assertEqual(42, blocking_async_task(foo))

    def test_successful_positional_args_operation(self):
        def foo(shift):
            return 42 + shift

        shift = 2
        self.assertEqual(42 + shift, blocking_async_task(foo, args=(shift,)))

    def test_successful_args_and_kwargs_operation(self):
        def foo(scale, shift):
            return scale*42 + shift

        scale, shift = 2, 4
        self.assertEqual(scale*42 + shift, blocking_async_task(foo, args=(scale,), kwargs={'shift': shift}))

    def test_unsuccessful_args_and_kwargs_operation_raises_exception(self):
        def foo(scale, shift):
            raise RuntimeError("Bad operation")

        scale, shift = 2, 4
        self.assertRaises(RuntimeError,
                          blocking_async_task, foo, args=(scale,), kwargs={'shift': shift})


if __name__ == "__main__":
    unittest.main()
