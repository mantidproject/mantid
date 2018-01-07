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
        success_cb_called = False
        error_cb_called = False
        task_exc = None

        def on_success(self, task_result):
            self.success_cb_called = True

        def on_error(self, exc):
            self.error_cb_called = True
            self.task_exc = exc

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

    def test_execute_raises_syntax_error_on_bad_code(self):
        code = "if:"
        self._verify_failed_serial_execute(SyntaxError, code, {}, {})

    def test_execute_async_calls_error_cb_on_syntax_error(self):
        code = "if:"
        executor = PythonCodeExecution()
        recv = PythonCodeExecutionTest.Receiver()
        task = executor.execute_async(code, {}, {}, error_cb=recv.on_error)
        task.join()
        self.assertTrue(recv.error_cb_called)
        self.assertTrue(isinstance(recv.task_exc, SyntaxError),
                        msg="Unexpected exception found. "
                            "SyntaxError expected, found {}".format(recv.task_exc.__class__.__name__))

    def test_execute_returns_failure_on_runtime_error_and_captures_exception(self):
        code = "x = _local + 1"
        self._verify_failed_serial_execute(NameError, code, {}, {})

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
