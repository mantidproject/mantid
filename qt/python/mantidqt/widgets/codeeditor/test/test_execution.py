# encoding: utf-8
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

# std imports
import sys
import traceback
import unittest

# 3rdparty imports
from qtpy.QtCore import QCoreApplication, QObject

# local imports
from mantid.py3compat import StringIO
from mantid.py3compat.mock import patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution, _get_imported_from_future, code_blocks


class Receiver(QObject):
    success_cb_called, error_cb_called = False, False
    task_exc, error_stack = None, None

    def on_success(self):
        self.success_cb_called = True

    def on_error(self, task_result):
        self.error_cb_called = True
        self.task_exc = task_result.exc_value
        self.error_stack = traceback.extract_tb(task_result.stack)


class ReceiverWithProgress(Receiver):

    def __init__(self):
        super(ReceiverWithProgress, self).__init__()
        self.lines_received = []

    def on_progess_update(self, lineno):
        self.lines_received.append(lineno)


@start_qapplication
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

    def test_startup_code_not_executed_by_default(self):
        executor = PythonCodeExecution(startup_code="x=100")
        self.assertFalse('x' in executor.globals_ns)

    # ---------------------------------------------------------------------------
    # Successful execution tests
    # ---------------------------------------------------------------------------
    def test_execute_places_output_in_globals(self):
        code = "_local=100"
        user_globals = self._verify_serial_execution_successful(code)
        self.assertEqual(100, user_globals['_local'])
        user_globals = self._verify_async_execution_successful(code)
        self.assertEqual(100, user_globals['_local'])

    def test_filename_sets__file__attr(self):
        executor = PythonCodeExecution()
        test_filename = 'script.py'
        executor.execute('x=1', filename=test_filename)
        self.assertTrue('__file__' in executor.globals_ns)
        self.assertEqual(test_filename, executor.globals_ns['__file__'])

    def test_empty_filename_does_not_set__file__attr(self):
        executor = PythonCodeExecution()
        executor.execute('x=1')
        self.assertTrue('__file__' not in executor.globals_ns)

    def test_execute_async_calls_success_signal_on_completion(self):
        code = "x=1+2"
        executor, recv = self._run_async_code(code)
        self.assertTrue(recv.success_cb_called)
        self.assertFalse(recv.error_cb_called)

    def test_script_dir_added_to_path_on_execution(self):
        code = "import sys; syspath = sys.path"
        test_filename = '/path/to/script/called/script.py'
        executor = PythonCodeExecution()
        executor.execute(code, filename=test_filename)
        self.assertIn('/path/to/script/called', executor.globals_ns['syspath'])

    def test_script_dir_removed_from_path_after_execution(self):
        code = "import sys; syspath = sys.path"
        test_filename = '/path/to/script/called/script.py'
        executor = PythonCodeExecution()
        executor.execute(code, filename=test_filename)
        self.assertNotIn('/path/to/script/called', sys.path)

    def test_get_imported_from_future_gets_imports_and_ignores_comments(self):
        code = ("from __future__ import division, print_function\n"
                "# from __future__ import unicode_literals\n")
        f_imports = _get_imported_from_future(code)
        self.assertEqual(['division', 'print_function'], f_imports)

    def test_future_division_active_when_running_script_with_future_import(self):
        global_var = "one_half"
        code = ("from __future__ import division\n"
                "{} = 1/2\n".format(global_var))
        executor = PythonCodeExecution()
        executor.execute(code)
        self.assertAlmostEqual(0.50, executor.globals_ns[global_var])

    @patch('sys.stdout', new_callable=StringIO)
    def test_future_print_function_active_in_scripts_if_imported(self, mock_stdout):
        code = ("from __future__ import division, print_function\n"
                "print('This', 'should', 'have', 'no', 'brackets')\n")
        executor = PythonCodeExecution()
        executor.execute(code)
        self.assertEqual("This should have no brackets\n", mock_stdout.getvalue())

    @patch('sys.stdout', new_callable=StringIO)
    def test_scripts_can_print_unicode_if_unicode_literals_imported(self, mock_stdout):
        code = ("from __future__ import unicode_literals\n"
                "print('£')\n")
        executor = PythonCodeExecution()
        executor.execute(code)
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
        self.assertEqual(5, len(recv.error_stack))
        # check line numbers
        self.assertEqual(8, recv.error_stack[2][1])
        self.assertEqual(7, recv.error_stack[3][1])
        self.assertEqual(5, recv.error_stack[4][1])

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
        self.assertEqual(filename, recv.error_stack[-1][0])

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


class CodeBlocksTest(unittest.TestCase):
    def test_block_splitting_simple_lines(self):
        code_str = r"""# make a dictionary from two lists
keys = ['a','b','c']
vals = list(range(3,9,2))
MyDict = dict(list(zip(keys,vals)))
print('\nKey ({0:s}) added to dictionary'.format(KeyToAdd))"""

        expected_blocks = [
            '# make a dictionary from two lists\n',
            "\nkeys = ['a','b','c']\n",
            '\n\nvals = list(range(3,9,2))\n',
            '\n\n\nMyDict = dict(list(zip(keys,vals)))\n',
            "\n\n\n\nprint('\\nKey ({0:s}) added to dictionary'.format(KeyToAdd))\n",
        ]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_string_handling(self):
        code_str = r"""a="\n\n\tlots of whitespace\nhere!"
b="inner 'quotes'"
c=r"raw\n\tstring\n\nwith\nwith\tspaces"
d=r'raw\n\tstring\n\nwith\nwith\tspaces but single\nquotes'"""

        expected_blocks = [
            r'a="\n\n\tlots of whitespace\nhere!"' + '\n',
            "\nb=\"inner 'quotes'\"" + '\n',
            '\n\n' + r'c=r"raw\n\tstring\n\nwith\nwith\tspaces"' + '\n',
            "\n\n\n" + r"d=r'raw\n\tstring\n\nwith\nwith\tspaces but single\nquotes'" + '\n'
        ]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_string_handling_multiline(self):
        code_str = """a=\"\"\"this
is
    a multine
        string
        comment
    with
varying
    indentation\"\"\"
print('and this is the next line')
print('''one line multiline ''')
print("and the last line")"""

        expected_blocks = [
            """a=\"\"\"this
is
    a multine
        string
        comment
    with
varying
    indentation\"\"\"
""",
            "\n\n\n\n\n\n\n\nprint('and this is the next line')\n",
            "\n\n\n\n\n\n\n\n\nprint('''one line multiline ''')\n",
            '\n\n\n\n\n\n\n\n\n\nprint("and the last line")\n',
        ]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_try_except(self):
        code_str = """print ("already defined:",dir())
try:
    x=UndefinedFunction4()
except:
    print ("need to define my own UndefinedFunction4")
def UndefinedFunction4(a=0.0):
        return a*2
"""
        # not currently ideal blocking, but it does not fail
        expected_blocks = [
            'print ("already defined:",dir())\n',
            """\ntry:
    x=UndefinedFunction4()
except:
    print ("need to define my own UndefinedFunction4")
def UndefinedFunction4(a=0.0):
        return a*2
"""]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_if(self):
        code_str = """if a==1:
    print(a)
elif b==1:
    print(b)
else:
    print (a+b)"""
        expected_blocks = [
            """if a==1:
    print(a)
elif b==1:
    print(b)
else:
    print (a+b)
"""]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_blank_lines(self):
        code_str = """print ("already defined:",dir())
try:
    x=UndefinedFunction4()
except:

    print ("need to define my own UndefinedFunction4")
    def UndefinedFunction4(a=0.0):
        return a*2
print(UndefinedFunction4(3))"""
        expected_blocks = [
            'print ("already defined:",dir())\n',
            """\ntry:
    x=UndefinedFunction4()
except:

    print ("need to define my own UndefinedFunction4")
    def UndefinedFunction4(a=0.0):
        return a*2
""",
            '\n\n\n\n\n\n\n\nprint(UndefinedFunction4(3))\n'
        ]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_2_level_indents(self):
        code_str = """for i in range(10):
    print(i)
    print(i+2)
    for j in range(4):
        print(j)
        #pointless comment
        a="nother line"
        
        we="left a blank space"
    back="to the original loop"

next='line after a gap'""" # noqa
        # not currently ideal blocking, but it does not fail
        expected_blocks = [
            """for i in range(10):
    print(i)
    print(i+2)
    for j in range(4):
        print(j)
        #pointless comment
        a="nother line"
        
        we="left a blank space"
    back="to the original loop"

next='line after a gap'
"""] # noqa
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_comment_lines(self):
        code_str = """# add a key if doesn't exist
KeyToAdd = 'apple' # this is unicode (default str type in python 3)
ValToAdd = 8.5
if KeyToAdd not in MyDict:
    # add the apple key
    MyDict[KeyToAdd] = ValToAdd
    print('Key ({0:s}) added to dictionary'.format(KeyToAdd))"""

        expected_blocks = [
            "# add a key if doesn't exist\n",
            "\nKeyToAdd = 'apple' # this is unicode (default str type in python 3)\n",
            '\n\nValToAdd = 8.5\n',
            """\n\n\nif KeyToAdd not in MyDict:
    # add the apple key
    MyDict[KeyToAdd] = ValToAdd
    print('Key ({0:s}) added to dictionary'.format(KeyToAdd))\n"""
        ]
        self._compare_block_splitting(code_str, expected_blocks)

    def test_block_splitting_handles_tabs(self):
        code_str = "if a=='this is a line with\tinternal\t tabs':\n" + \
                        "\tprint('another with\tinternal\t tabs')\n"

        # just the tab preceeding print is converted
        expected_blocks = ["if a=='this is a line with\tinternal\t tabs':\n" +
                                "    print('another with\tinternal\t tabs')\n"]
        self._compare_block_splitting(code_str, expected_blocks)

    def _compare_block_splitting(self, code_str, expected_blocks):
        block_num = 0
        for block in code_blocks(code_str):
            self.assertEqual(expected_blocks[block_num], block.code_str)
            block_num += 1
        self.assertEqual(len(expected_blocks), block_num)

if __name__ == "__main__":
    unittest.main()
