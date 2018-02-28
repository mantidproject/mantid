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

# 3rdparty imports
from qtpy.QtCore import QCoreApplication, QObject

# local imports
from mantidqt.utils.qt.testing import requires_qapp
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution


class Receiver(QObject):
    success_cb_called, error_cb_called = False, False
    task_exc, error_stack = None, None

    def on_success(self):
        self.success_cb_called = True

    def on_error(self, task_result):
        self.error_cb_called = True
        self.task_exc = task_result.exc_value
        self.error_stack = task_result.stack


class ReceiverWithProgress(Receiver):

    def __init__(self):
        super(ReceiverWithProgress, self).__init__()
        self.lines_received = []

    def on_progess_update(self, lineno):
        self.lines_received.append(lineno)


@requires_qapp
class PythonCodeExecutionTest(unittest.TestCase):

    def test_default_construction_yields_empty_context(self):
        executor = PythonCodeExecution()
        self.assertEqual(0, len(executor.globals_ns))

    def test_reset_context_clears_context(self):
        executor = PythonCodeExecution()
        globals_len = len(executor.globals_ns)
        executor.execute("x = 1")
        self.assertTrue(globals_len + 1, len(executor.globals_ns))
        executor.reset_context()
        self.assertEqual(0, len(executor.globals_ns))

    def test_startup_code_executed_by_default(self):
        executor = PythonCodeExecution(startup_code="x=100")
        self.assertEqual(100, executor.globals_ns['x'])

    # ---------------------------------------------------------------------------
    # Successful execution tests
    # ---------------------------------------------------------------------------
    def test_execute_places_output_in_globals(self):
        code = "_local=100"
        user_globals = self._verify_serial_execution_successful(code)
        self.assertEquals(100, user_globals['_local'])
        user_globals = self._verify_async_execution_successful(code)
        self.assertEquals(100, user_globals['_local'])

    def test_execute_async_calls_success_signal_on_completion(self):
        code = "x=1+2"
        executor, recv = self._run_async_code(code)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)

    # ---------------------------------------------------------------------------
    # Error execution tests
    # ---------------------------------------------------------------------------
    def test_execute_raises_syntax_error_on_bad_code(self):
        code = "if:"
        self._verify_failed_serial_execute(SyntaxError, code)

    def test_execute_async_calls_error_cb_on_syntax_error(self):
        code = "if:"
        executor, recv = self._run_async_code(code)

        self.assertTrue(recv.error_cb_called)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(isinstance(recv.task_exc, SyntaxError),
                        msg="Unexpected exception found. "
                            "SyntaxError expected, found {}".format(recv.task_exc.__class__.__name__))
        self.assertEqual(1, recv.task_exc.lineno)

    def test_execute_returns_failure_on_runtime_error_and_captures_exception(self):
        code = "x = _local + 1"
        self._verify_failed_serial_execute(NameError, code)

    def test_execute_async_returns_failure_on_runtime_error_and_captures_expected_stack(self):
        code = """
def foo():
    def bar():
        \"""raises a NameError\"""
        y = _local + 1
    # call inner
    bar()
foo()
"""
        executor, recv = self._run_async_code(code, with_progress=True)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, NameError),
                        msg="Unexpected exception found. "
                            "NameError expected, found {}".format(recv.task_exc.__class__.__name__))
        # Test the stack has been chopped as expected
        self.assertEqual(3, len(recv.error_stack))
        # check line numbers
        self.assertEqual(8, recv.error_stack[0][1])
        self.assertEqual(7, recv.error_stack[1][1])
        self.assertEqual(5, recv.error_stack[2][1])

    # ---------------------------------------------------------------------------
    # Progress tests
    # ---------------------------------------------------------------------------
    def test_progress_cb_is_not_called_for_empty_string(self):
        code = ""
        executor, recv = self._run_async_code(code, with_progress=True)
        self.assertEqual(0, len(recv.lines_received))

    def test_progress_cb_is_not_called_for_code_with_syntax_errors(self):
        code = """x = 1
y =
"""
        executor, recv = self._run_async_code(code, with_progress=True)
        self.assertEqual(0, len(recv.lines_received))
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertEqual(0, len(recv.lines_received))

    def test_progress_cb_is_called_for_single_line(self):
        code = "x = 1"
        executor, recv = self._run_async_code(code, with_progress=True)
        if not recv.success_cb_called:
            self.assertTrue(recv.error_cb_called)
            self.fail("Execution failed with error:\n" + str(recv.task_exc))

        self.assertEqual([1], recv.lines_received)

    def test_progress_cb_is_called_for_multiple_single_lines(self):
        code = """x = 1
y = 2
"""
        executor, recv = self._run_async_code(code, with_progress=True)
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
        executor, recv = self._run_async_code(code, with_progress=True)
        if not recv.success_cb_called:
            if recv.error_cb_called:
                self.fail("Unexpected error found: " + str(recv.task_exc))
            else:
                self.fail("No callback was called!")

        context = executor.globals_ns
        self.assertEqual(20, context['sum'])
        self.assertEqual(20*20, context['squared'])
        self.assertEqual(1, context['x'])
        self.assertEqual([1, 2, 3, 4, 9], recv.lines_received)

    # -------------------------------------------------------------------------
    # Filename checks
    # -------------------------------------------------------------------------
    def test_filename_included_in_traceback_if_supplied(self):
        code = """raise RuntimeError"""
        filename = 'test.py'
        executor, recv = self._run_async_code(code, filename=filename)
        self.assertTrue(recv.error_cb_called)
        self.assertEqual(filename, recv.error_stack[0][0])

    # -------------------------------------------------------------------------
    # Helpers
    # -------------------------------------------------------------------------
    def _verify_serial_execution_successful(self, code):
        executor = PythonCodeExecution()
        executor.execute(code)
        return executor.globals_ns

    def _verify_async_execution_successful(self, code):
        executor = PythonCodeExecution()
        task = executor.execute_async(code)
        task.join()
        return executor.globals_ns

    def _verify_failed_serial_execute(self, expected_exc_type, code):
        executor = PythonCodeExecution()
        self.assertRaises(expected_exc_type, executor.execute, code)

    def _run_async_code(self, code, with_progress=False, filename=None):
        executor = PythonCodeExecution()
        if with_progress:
            recv = ReceiverWithProgress()
            executor.sig_exec_progress.connect(recv.on_progess_update)
        else:
            recv = Receiver()
        executor.sig_exec_success.connect(recv.on_success)
        executor.sig_exec_error.connect(recv.on_error)
        task = executor.execute_async(code, filename)
        task.join()
        QCoreApplication.processEvents()

        return executor, recv


if __name__ == "__main__":
    unittest.main()
