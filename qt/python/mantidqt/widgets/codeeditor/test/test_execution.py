#  This file is part of the mantidqt package
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
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# std imports
import unittest

# local imports
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution


class PythonCodeExecutionTest(unittest.TestCase):

    class Receiver(object):
        success_cb_called, error_cb_called = False, False
        task_exc, task_tb = None, None

        def on_success(self):
            self.success_cb_called = True

        def on_error(self, task_result):
            self.error_cb_called = True
            self.task_exc = task_result.exception
            self.task_tb = task_result.traceback

    class ReceiverWithProgress(Receiver):

        def __init__(self):
            self.lines_received = []

        def on_progess_update(self, lineno):
            self.lines_received.append(lineno)

        def on_error(self, exc):
            self.error_cb_called = True
            self.task_exc = exc

    # ---------------------------------------------------------------------------
    # Successful execution tests
    # ---------------------------------------------------------------------------
    def test_execute_places_output_in_provided_mapping_object(self):
        code = "_local=100"
        namespace = {}
        self._verify_serial_execution_successful(code, namespace, namespace)
        namespace = {}
        self._verify_async_execution_successful(code, namespace, namespace)
        self.assertEquals(100, namespace['_local'])

    def test_execute_places_output_in_locals_mapping_if_different_to_globals(self):
        code = "_local=100"
        user_globals, user_locals = {}, {}
        self._verify_serial_execution_successful(code, user_globals, user_locals)
        user_globals, user_locals = {}, {}
        self._verify_async_execution_successful(code, user_globals, user_locals)
        self.assertEquals(100, user_locals['_local'])

    def test_execute_async_calls_success_cb_on_completion(self):
        code = "if:"
        recv = PythonCodeExecutionTest.Receiver()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error)
        task = executor.execute_async(code, {}, {})
        task.join()

    # ---------------------------------------------------------------------------
    # Error execution tests
    # ---------------------------------------------------------------------------
    def test_execute_raises_syntax_error_on_bad_code(self):
        code = "if:"
        self._verify_failed_serial_execute(SyntaxError, code, {}, {})

    def test_execute_async_calls_error_cb_on_syntax_error(self):
        code = "if:"
        recv = PythonCodeExecutionTest.Receiver()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error)
        task = executor.execute_async(code, {}, {})
        task.join()
        self.assertTrue(recv.error_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(isinstance(recv.task_exc, SyntaxError),
                        msg="Unexpected exception found. "
                            "SyntaxError expected, found {}".format(recv.task_exc.__class__.__name__))
        self.assertEqual(1, recv.task_tb.tb_lineno)

    def test_execute_returns_failure_on_runtime_error_and_captures_exception(self):
        code = "x = _local + 1"
        self._verify_failed_serial_execute(NameError, code, {}, {})

    def test_execute_async_returns_failure_on_runtime_error_and_captures_exception(self):
        code = """x = + 1
y = _local + 1        
"""
        recv = PythonCodeExecutionTest.Receiver()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error)
        task = executor.execute_async(code, {}, {})
        task.join()
        self.assertTrue(recv.error_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(isinstance(recv.task_exc, NameError),
                        msg="Unexpected exception found. "
                            "NameError expected, found {}".format(recv.task_exc.__class__.__name__))
        self.assertEqual(2, recv.task_tb.tb_lineno)

    # ---------------------------------------------------------------------------
    # Progress tests
    # ---------------------------------------------------------------------------
    def test_progress_cb_is_not_called_for_empty_string(self):
        code = ""
        recv = PythonCodeExecutionTest.ReceiverWithProgress()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error,
                                       progress_cb=recv.on_progess_update)
        task = executor.execute_async(code, {}, {})
        task.join()
        self.assertEqual(0, len(recv.lines_received))

    def test_progress_cb_is_not_called_for_code_with_syntax_errors(self):
        code = """x = 1
y = 
"""
        recv = PythonCodeExecutionTest.ReceiverWithProgress()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error,
                                       progress_cb=recv.on_progess_update)
        task = executor.execute_async(code, {}, {})
        task.join()
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertEqual(0, len(recv.lines_received))

    def test_progress_cb_is_called_for_single_line(self):
        code = "x = 1"
        recv = PythonCodeExecutionTest.ReceiverWithProgress()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error,
                                       progress_cb=recv.on_progess_update)
        task = executor.execute_async(code, {}, {})
        task.join()
        if not recv.success_cb_called:
            self.assertTrue(recv.error_cb_called)
            self.fail("Execution failed with error:\n" + str(recv.task_exc))

        self.assertEqual([1], recv.lines_received)

    def test_progress_cb_is_called_for_multiple_single_lines(self):
        code = """x = 1
y = 2
"""
        recv = PythonCodeExecutionTest.ReceiverWithProgress()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error,
                                       progress_cb=recv.on_progess_update)
        task = executor.execute_async(code, {}, {})
        task.join()
        if not recv.success_cb_called:
            self.assertTrue(recv.error_cb_called)
            self.fail("Execution failed with error:\n" + str(recv.task_exc))

        self.assertEqual([1, 2], recv.lines_received)

    def test_progress_cb_is_called_for_mix_single_lines_and_blocks(self):
        code = """x = 1
# comment line

sum = 0
for i in range(10):
    if i %2 == 0:
        sum += i

squared = sum*sum
"""
        recv = PythonCodeExecutionTest.ReceiverWithProgress()
        executor = PythonCodeExecution(success_cb=recv.on_success, error_cb=recv.on_error,
                                       progress_cb=recv.on_progess_update)
        context = {}
        task = executor.execute_async(code, context, context)
        task.join()
        if not recv.success_cb_called:
            self.assertTrue(recv.error_cb_called)
            self.fail("Execution failed with error:\n" + str(recv.task_exc))

        self.assertEqual(20, context['sum'])
        self.assertEqual(20*20, context['squared'])
        self.assertEqual(1, context['x'])
        self.assertEqual([1, 2, 3, 4, 5, 8, 9], recv.lines_received)

    # -------------------------------------------------------------------------
    # Helpers
    # -------------------------------------------------------------------------
    def _verify_serial_execution_successful(self, code, globals_ns,
                                            locals_ns):
        executor = PythonCodeExecution()
        executor.execute(code, globals_ns, locals_ns)

    def _verify_async_execution_successful(self, code, globals_ns,
                                           locals_ns):
        executor = PythonCodeExecution()
        task = executor.execute_async(code, globals_ns, locals_ns)
        task.join()

    def _verify_failed_serial_execute(self, expected_exc_type, code,
                                      globals_ns, locals_ns):
        executor = PythonCodeExecution()
        self.assertRaises(expected_exc_type, executor.execute, code, globals_ns, locals_ns)


if __name__ == "__main__":
    unittest.main()
