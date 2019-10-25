# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import absolute_import, unicode_literals

import __future__
import ast
import os

from qtpy.QtCore import QObject, Signal
from qtpy.QtWidgets import QApplication

from mantidqt.utils import AddedToSysPath
from mantidqt.utils.asynchronous import AsyncTask, BlockingAsyncTaskWithCallback
from mantidqt.widgets.codeeditor.inputsplitter import InputSplitter

EMPTY_FILENAME_ID = '<string>'
FILE_ATTR = '__file__'
COMPILE_MODE = 'exec'


def _get_imported_from_future(code_str):
    """
    Parse the given code and return a list of names that are imported
    from __future__.
    :param code_str: The code to parse
    :return list: List of names that are imported from __future__
    """
    future_imports = []
    for node in ast.walk(ast.parse(code_str)):
        if isinstance(node, ast.ImportFrom):
            if node.module == '__future__':
                future_imports.extend([import_alias.name for import_alias in node.names])
    return future_imports


def get_future_import_compiler_flags(code_str):
    """
    Get the compiler flags that can be passed to `compile` that
    correspond to the __future__ imports inside the given code.

    :param code_str: The code being executed, containing __future__ imports
    :return int: The 'bitwise or' union of compiler flags
    """
    flags = 0
    for f_import_str in _get_imported_from_future(code_str):
        try:
            future_import = getattr(__future__, f_import_str)
            flags |= future_import.compiler_flag
        except AttributeError:
            # Just pass and let the ImportError be raised on script execution
            pass
    return flags


class PythonCodeExecution(QObject):
    """Provides the ability to execute arbitrary
    strings of Python code. It supports
    reporting progress updates in asynchronous execution
    """
    sig_exec_success = Signal(object)
    sig_exec_error = Signal(object)
    sig_exec_progress = Signal(int)

    def __init__(self, startup_code=None):
        """Initialize the object"""
        super(PythonCodeExecution, self).__init__()

        self._globals_ns = None

        self._task = None

        self.reset_context()

        # the code is not executed initially so code completion won't work
        # on variables until part is executed

    @property
    def globals_ns(self):
        return self._globals_ns

    def abort(self):
        if self._task is not None:
            self._task.abort()

    def execute_async(self, code_str, filename=None, blocking=False):
        """
        Execute the given code string on a separate thread. This function
        returns as soon as the new thread starts

        :param code_str: A string containing code to execute
        :param filename: See PythonCodeExecution.execute()
        :param blocking: If True the call will block until the task is finished
        :returns: The created async task, only returns task if the blocking is False
        """
        # Stack is chopped on error to avoid the  AsyncTask.run->self.execute calls appearing
        # as these are not useful for the user in this context
        if not blocking:
            task = AsyncTask(self.execute, args=(code_str, filename),
                             success_cb=self._on_success, error_cb=self._on_error)
            task.start()
            self._task = task
            return task
        else:
            self._task = BlockingAsyncTaskWithCallback(self.execute, args=(code_str, filename),
                                                       success_cb=self._on_success, error_cb=self._on_error,
                                                       blocking_cb=QApplication.processEvents)
            return self._task.start()

    def execute(self, code_str, filename=None):
        """Execute the given code on the calling thread
        within the provided context.

        :param code_str: A string containing code to execute
        :param filename: An optional identifier specifying the file source of the code. If None then '<string>'
        is used
        :raises: Any error that the code generates
        """
        if filename:
            self.globals_ns[FILE_ATTR] = filename
        else:
            filename = EMPTY_FILENAME_ID
        flags = get_future_import_compiler_flags(code_str)
        # This line checks the whole code string for syntax errors, so that no
        # code blocks are executed if the script has invalid syntax.
        compile(code_str, filename, mode=COMPILE_MODE, dont_inherit=True, flags=flags)

        with AddedToSysPath([os.path.dirname(filename)]):
            sig_progress = self.sig_exec_progress
            for block in code_blocks(code_str):
                sig_progress.emit(block.lineno)
                # compile so we can set the filename
                code_obj = compile(block.code_str, filename, mode=COMPILE_MODE,
                                   dont_inherit=True, flags=flags)
                exec (code_obj, self.globals_ns, self.globals_ns)

    def reset_context(self):
        # create new context for execution
        self._globals_ns, self._namespace = {}, {}

    # --------------------- Callbacks -------------------------------
    def _on_success(self, task_result):
        self._reset_task()
        self.sig_exec_success.emit(task_result)

    def _on_error(self, task_error):
        self._reset_task()
        self.sig_exec_error.emit(task_error)

    def _on_progress_updated(self, lineno):
        self.sig_exec_progress(lineno)

    # --------------------- Private -------------------------------
    def _reset_task(self):
        self._task = None


class CodeBlock(object):
    """Holds an executable code object. It also stores the line number
    of the first line within a larger group of code blocks"""

    def __init__(self, code_str, lineno):
        self.code_str = code_str
        self.lineno = lineno


def code_blocks(code_str):
    """Generator to produce blocks of executable code
    from the given code string.
    """
    lineno_cur = 0
    lines = code_str.splitlines()
    line_count = len(lines)
    isp = InputSplitter()
    for line in lines:
        lineno_cur += 1
        # IPython InputSplitter assumes that indentation is 4 spaces, not tabs.
        # Accounting for that here, rather than using script-level "tabs to spaces"
        # allows the user to keep tabs in their script if they wish.
        line = line.replace("\t", " "*4)
        isp.push(line)
        # If we need more input to form a complete statement
        # or we are not at the end of the code then keep
        # going
        if isp.push_accepts_more() and lineno_cur != line_count:
            continue
        else:
            # Now we have a complete set of executable statements
            # throw them at the execution engine
            code = isp.source
            isp.reset()
            yield CodeBlock(code, lineno_cur)
            # In order to keep the line numbering in error stack traces
            # consistent each executed block needs to have the statements
            # on the same line as they are in the real code so we prepend
            # blank lines to make this so
            isp.push('\n' * lineno_cur)
