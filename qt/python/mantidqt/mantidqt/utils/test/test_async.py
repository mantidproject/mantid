# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# system imports
import traceback
import unittest

# 3rdparty imports

# local imports
from mantidqt.utils.asynchronous import AsyncTask, BlockingAsyncTaskWithCallback


class AsyncTaskTest(unittest.TestCase):
    class Receiver(object):
        success_cb_called, error_cb_called, finished_cb_called = False, False, False
        task_output = (None,)
        task_exc_type, task_exc, task_exc_stack = None, None, None

        def on_success(self, task_result):
            self.success_cb_called = True
            self.task_output = task_result.output

        def on_error(self, task_result):
            self.error_cb_called = True
            self.task_exc_type = task_result.exc_type
            self.task_exc = task_result.exc_value
            self.task_exc_stack = traceback.extract_tb(task_result.stack)

        def on_finished(self):
            self.finished_cb_called = True

    # ---------------------------------------------------------------
    # Success cases
    # ---------------------------------------------------------------
    def test_successful_no_arg_operation_calls_success_and_finished_callback(self):
        def foo():
            return 42

        recv = AsyncTaskTest.Receiver()
        t = AsyncTask(foo, success_cb=recv.on_success, error_cb=recv.on_error, finished_cb=recv.on_finished)
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
        t = AsyncTask(foo, args=(shift,), success_cb=recv.on_success, error_cb=recv.on_error, finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)
        self.assertEqual(42 + shift, recv.task_output)

    def test_successful_args_and_kwargs_operation_calls_success_and_finished_callback(self):
        def foo(scale, shift):
            return scale * 42 + shift

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(
            foo, args=(scale,), kwargs={"shift": shift}, success_cb=recv.on_success, error_cb=recv.on_error, finished_cb=recv.on_finished
        )
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)
        self.assertEqual(scale * 42 + shift, recv.task_output)

    def test_unsuccessful_no_arg_operation_calls_error_and_finished_callback(self):
        def foo():
            # this is a bad operation
            # that should appear in the stack trace
            raise RuntimeError("Bad operation")

        recv = AsyncTaskTest.Receiver()
        t = AsyncTask(foo, success_cb=recv.on_success, error_cb=recv.on_error, finished_cb=recv.on_finished)
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError), msg="Expected RuntimeError, found " + recv.task_exc.__class__.__name__)

        self.assertEqual(2, len(recv.task_exc_stack))
        # line number of self.target in asynchronous.py
        self.assertEqual(63, recv.task_exc_stack[0][1])
        # line number of raise statement above
        self.assertEqual(89, recv.task_exc_stack[1][1])

    def test_unsuccessful_args_and_kwargs_operation_calls_error_and_finished_callback(self):
        def foo(scale, shift):
            raise RuntimeError("Bad operation")

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(
            foo, args=(scale,), kwargs={"shift": shift}, success_cb=recv.on_success, error_cb=recv.on_error, finished_cb=recv.on_finished
        )
        t.start()
        t.join()
        self.assertTrue(recv.finished_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError))

    def test_unsuccessful_operation_with_error_cb(self):
        def foo(scale, shift):
            def bar():
                raise RuntimeError("Bad operation")

            bar()

        recv = AsyncTaskTest.Receiver()
        scale, shift = 2, 4
        t = AsyncTask(foo, args=(scale,), kwargs={"shift": shift}, error_cb=recv.on_error)
        t.start()
        t.join()
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, RuntimeError))
        self.assertEqual(3, len(recv.task_exc_stack))
        # line number of exception
        self.assertEqual(127, recv.task_exc_stack[1][1])
        # line number of exception
        self.assertEqual(125, recv.task_exc_stack[2][1])

    # ---------------------------------------------------------------
    # Failure cases
    # ---------------------------------------------------------------
    def test_non_callable_object_raises_exception(self):
        self.assertRaises(TypeError, AsyncTask, None)
        self.assertRaises(TypeError, AsyncTask, object())


class BlockingAsyncTaskWithCallbackTest(unittest.TestCase):
    # ---------------------------------------------------------------
    # Success cases
    # ---------------------------------------------------------------
    def test_successful_no_arg_operation_without_callback(self):
        def foo():
            return 42

        task = BlockingAsyncTaskWithCallback(foo)

        self.assertEqual(42, task.start())

    def test_successful_positional_args_operation(self):
        def foo(shift):
            return 42 + shift

        shift = 2
        task = BlockingAsyncTaskWithCallback(foo, args=(shift,))

        self.assertEqual(42 + shift, task.start())

    def test_successful_args_and_kwargs_operation(self):
        def foo(scale, shift):
            return scale * 42 + shift

        scale, shift = 2, 4

        task = BlockingAsyncTaskWithCallback(foo, args=(scale,), kwargs={"shift": shift})
        self.assertEqual(scale * 42 + shift, task.start())

    def test_unsuccessful_args_and_kwargs_operation_raises_exception(self):
        def foo(scale, shift):
            raise RuntimeError("Bad operation")

        scale, shift = 2, 4
        task = BlockingAsyncTaskWithCallback(foo, args=(scale,), kwargs={"shift": shift})
        self.assertRaises(RuntimeError, task.start)


if __name__ == "__main__":
    unittest.main()
