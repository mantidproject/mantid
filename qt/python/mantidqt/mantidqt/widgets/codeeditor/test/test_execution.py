# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# encoding: utf-8
#  This file is part of the mantidqt package
#
#
# std imports
import sys
import traceback
import unittest

# 3rdparty imports
from qtpy.QtCore import QCoreApplication, QObject

# local imports
from io import StringIO
from unittest.mock import patch, Mock

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution, _get_imported_from_future


class Receiver(QObject):
    success_cb_called, error_cb_called = False, False
    task_exc, error_stack = None, None

    def on_success(self):
        self.success_cb_called = True

    def on_error(self, task_result):
        self.error_cb_called = True
        self.task_exc = task_result.exc_value
        self.error_stack = traceback.extract_tb(task_result.stack)


@start_qapplication
class PythonCodeExecutionTest(unittest.TestCase):
    def test_default_construction_context_contains_builtins(self):
        executor = PythonCodeExecution()
        self.assertTrue("__builtins__" in executor.globals_ns)

    def test_reset_context_remove_user_content(self):
        executor = PythonCodeExecution()
        executor.execute("x = 1", line_offset=0)
        self.assertTrue("x" in executor.globals_ns)
        executor.reset_context()

        self.assertFalse("x" in executor.globals_ns)
        self.assertTrue("__builtins__" in executor.globals_ns)

    # ---------------------------------------------------------------------------
    # Successful execution tests
    # ---------------------------------------------------------------------------
    def test_execute_places_output_in_globals(self):
        code = "_local=100"
        user_globals = self._verify_serial_execution_successful(code)
        self.assertEqual(100, user_globals["_local"])
        user_globals = self._verify_async_execution_successful(code)
        self.assertEqual(100, user_globals["_local"])

    def test_filename_sets__file__attr(self):
        executor = PythonCodeExecution()
        test_filename = "script.py"
        executor.execute("x=1", filename=test_filename, line_offset=0)
        self.assertTrue("__file__" in executor.globals_ns)
        self.assertEqual(test_filename, executor.globals_ns["__file__"])

    def test_empty_filename_sets_identifier(self):
        executor = PythonCodeExecution()
        executor.execute("x=1", line_offset=0)
        self.assertTrue("__file__" in executor.globals_ns)

    def test_execute_async_calls_success_signal_on_completion(self):
        code = "x=1+2"
        executor, recv = self._run_async_code(code)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)

    def test_script_dir_added_to_path_on_execution(self):
        code = "import sys; syspath = sys.path"
        test_filename = "/path/to/script/called/script.py"
        executor = PythonCodeExecution()
        executor.execute(code, filename=test_filename, line_offset=0)
        self.assertIn("/path/to/script/called", executor.globals_ns["syspath"])

    def test_script_dir_removed_from_path_after_execution(self):
        code = "import sys; syspath = sys.path"
        test_filename = "/path/to/script/called/script.py"
        executor = PythonCodeExecution()
        executor.execute(code, filename=test_filename, line_offset=0)
        self.assertNotIn("/path/to/script/called", sys.path)

    def test_line_offset_forwarded_on(self):
        executor = PythonCodeExecution()
        code = "x=0"
        offset = 10
        with patch("mantidqt.widgets.codeeditor.execution.CodeExecution") as patched_constructor:
            mocked_executor = Mock()
            patched_constructor.return_value = mocked_executor

            executor.execute(code, line_offset=offset)
            self.assertTrue(mocked_executor.execute.called)
            args, _ = mocked_executor.execute.call_args_list[0]
            self.assertTrue(offset in args, "Line offset was not passed in")

    def test_get_imported_from_future_gets_imports_and_ignores_comments(self):
        code = "from __future__ import division, print_function\n# from __future__ import unicode_literals\n"
        f_imports = _get_imported_from_future(code)
        self.assertEqual(["division", "print_function"], f_imports)

    def test_future_division_active_when_running_script_with_future_import(self):
        global_var = "one_half"
        code = "from __future__ import division\n{} = 1/2\n".format(global_var)
        executor = PythonCodeExecution()
        executor.execute(code, line_offset=0)
        self.assertAlmostEqual(0.50, executor.globals_ns[global_var])

    @patch("sys.stdout", new_callable=StringIO)
    def test_future_print_function_active_in_scripts_if_imported(self, mock_stdout):
        code = "from __future__ import division, print_function\nprint('This', 'should', 'have', 'no', 'brackets')\n"
        executor = PythonCodeExecution()
        executor.execute(code, line_offset=0)
        self.assertEqual("This should have no brackets\n", mock_stdout.getvalue())

    @patch("sys.stdout", new_callable=StringIO)
    def test_scripts_can_print_unicode_if_unicode_literals_imported(self, mock_stdout):
        code = "from __future__ import unicode_literals\nprint('£')\n"
        executor = PythonCodeExecution()
        executor.execute(code, line_offset=0)
        self.assertEqual("£\n", mock_stdout.getvalue())

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
        self.assertTrue(
            isinstance(recv.task_exc, SyntaxError),
            msg="Unexpected exception found. SyntaxError expected, found {}".format(recv.task_exc.__class__.__name__),
        )
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
        executor, recv = self._run_async_code(code)
        self.assertFalse(recv.success_cb_called)
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(
            isinstance(recv.task_exc, NameError),
            msg="Unexpected exception found. NameError expected, found {}".format(recv.task_exc.__class__.__name__),
        )
        # Test the stack has been chopped as expected
        self.assertEqual(5, len(recv.error_stack))
        # check line numbers
        self.assertEqual(8, recv.error_stack[2][1])
        self.assertEqual(7, recv.error_stack[3][1])
        self.assertEqual(5, recv.error_stack[4][1])

    # -------------------------------------------------------------------------
    # Filename checks
    # -------------------------------------------------------------------------
    def test_filename_included_in_traceback_if_supplied(self):
        code = """raise RuntimeError"""
        filename = "test.py"
        executor, recv = self._run_async_code(code, filename=filename)
        self.assertTrue(recv.error_cb_called)
        self.assertEqual(filename, recv.error_stack[-1][0])

    # -------------------------------------------------------------------------
    # Helpers
    # -------------------------------------------------------------------------
    def _verify_serial_execution_successful(self, code):
        executor = PythonCodeExecution()
        executor.execute(code)
        return executor.globals_ns

    def _verify_async_execution_successful(self, code, line_offset=0):
        executor = PythonCodeExecution()
        task = executor.execute_async(code, line_offset)
        task.join()
        return executor.globals_ns

    def _verify_failed_serial_execute(self, expected_exc_type, code):
        executor = PythonCodeExecution()
        self.assertRaises(expected_exc_type, executor.execute, code)

    def _run_async_code(self, code, filename=None, line_no=0):
        executor = PythonCodeExecution()
        recv = Receiver()
        executor.sig_exec_success.connect(recv.on_success)
        executor.sig_exec_error.connect(recv.on_error)
        task = executor.execute_async(code, line_no, filename)
        task.join()
        QCoreApplication.sendPostedEvents()

        return executor, recv


if __name__ == "__main__":
    unittest.main()
