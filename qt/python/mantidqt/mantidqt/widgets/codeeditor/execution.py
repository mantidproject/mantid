# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
import __future__
import ast
import warnings

try:
    import builtins
except ImportError:
    import __main__

    builtins = __main__.__builtins__
import copy
import os
from io import BytesIO
from lib2to3.pgen2.tokenize import detect_encoding

from qtpy.QtCore import QObject, Signal
from qtpy.QtWidgets import QApplication

from mantidqt.utils import AddedToSysPath
from mantidqt.utils.asynchronous import AsyncTask, BlockingAsyncTaskWithCallback
from mantidqt.utils.qt import import_qt

# Core object to execute the code with optinal progress tracking
CodeExecution = import_qt("..._common", "mantidqt.widgets.codeeditor", "CodeExecution")

EMPTY_FILENAME_ID = "<string>"
FILE_ATTR = "__file__"


def _get_imported_from_future(code_str):
    """
    Parse the given code and return a list of names that are imported
    from __future__.
    :param code_str: The code to parse
    :return list: List of names that are imported from __future__
    """
    future_imports = []
    try:
        code_str = code_str.encode(detect_encoding(BytesIO(code_str.encode()).readline)[0])
    except UnicodeEncodeError:  # Script contains unicode symbol. Cannot run detect_encoding as it requires ascii.
        code_str = code_str.encode("utf-8")
    for node in ast.walk(ast.parse(code_str)):
        if isinstance(node, ast.ImportFrom):
            if node.module == "__future__":
                future_imports.extend([import_alias.name for import_alias in node.names])
                break
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


def hide_warnings_in_script_editor():
    warnings.filterwarnings("ignore", message="Starting a Matplotlib GUI outside of the main thread will likely fail.")


class PythonCodeExecution(QObject):
    """Provides the ability to execute arbitrary
    strings of Python code. It supports
    reporting progress updates in asynchronous execution
    """

    sig_exec_success = Signal(object)
    sig_exec_error = Signal(object)

    def __init__(self, editor=None):
        """Initialize the object"""
        super(PythonCodeExecution, self).__init__()
        self._editor = editor
        self._globals_ns = None
        self._task = None
        self.reset_context()

    @property
    def globals_ns(self):
        return self._globals_ns

    def abort(self):
        if self._task is not None:
            self._task.abort()

    def execute_async(self, code_str, line_offset, filename=None, blocking=False):
        """
        Execute the given code string on a separate thread. This function
        returns as soon as the new thread starts

        :param code_str: A string containing code to execute
        :param line_offset: See PythonCodeExecution.execute()
        :param filename: See PythonCodeExecution.execute()
        :param blocking: If True the call will block until the task is finished
        :returns: The created async task, only returns task if the blocking is False
        """
        # Stack is chopped on error to avoid the  AsyncTask.run->self.execute calls appearing
        # as these are not useful for the user in this context
        if not blocking:
            task = AsyncTask(self.execute, args=(code_str, filename, line_offset), success_cb=self._on_success, error_cb=self._on_error)
            task.start()
            self._task = task
            return task
        else:
            self._task = BlockingAsyncTaskWithCallback(
                self.execute,
                args=(code_str, filename, line_offset),
                success_cb=self._on_success,
                error_cb=self._on_error,
                blocking_cb=QApplication.processEvents,
            )
            return self._task.start()

    def execute(self, code_str, filename=None, line_offset=0):
        """Execute the given code on the calling thread
        within the provided context.

        :param code_str: A string containing code to execute
        :param filename: An optional identifier specifying the file source of the code. If None then '<string>'
        is used
        :param line_offset: The number of lines offset into the script that the code_str starts at
        :raises: Any error that the code generates
        """
        if not filename:
            filename = EMPTY_FILENAME_ID

        self.globals_ns[FILE_ATTR] = filename
        flags = get_future_import_compiler_flags(code_str)
        with AddedToSysPath([os.path.dirname(filename)]):
            executor = CodeExecution(self._editor)
            hide_warnings_in_script_editor()
            executor.execute(code_str, filename, flags, self.globals_ns, line_offset)

    def reset_context(self):
        # create new context for execution
        self._globals_ns = copy.copy(builtins.globals())

    # --------------------- Callbacks -------------------------------
    def _on_success(self, task_result):
        self._reset_task()
        self.sig_exec_success.emit(task_result)

    def _on_error(self, task_error):
        self._reset_task()
        self.sig_exec_error.emit(task_error)

    # --------------------- Private -------------------------------
    def _reset_task(self):
        self._task = None
