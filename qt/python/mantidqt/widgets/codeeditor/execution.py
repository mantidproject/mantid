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
import ctypes
import time

# 3rdparty imports
from qtpy.QtCore import QObject, Signal

# local imports
from mantidqt.widgets.codeeditor.inputsplitter import InputSplitter
from mantidqt.utils.async import AsyncTask


class PythonCodeExecution(QObject):
    """Provides the ability to execute arbitrary
    strings of Python code. It supports
    reporting progress updates in asynchronous execution
    """
    sig_exec_success = Signal(object)
    sig_exec_error = Signal(object)
    sig_exec_progress = Signal(int)

    def __init__(self):
        """Initialize the object"""
        super(PythonCodeExecution, self).__init__()

        self._globals_ns, self._locals_ns = None, None
        self._task = None

        self.reset_context()

    @property
    def globals_ns(self):
        return self._globals_ns

    @property
    def locals_ns(self):
        return self._locals_ns

    def abort(self):
        """Cancel an asynchronous execution"""
        # Implementation is based on
        # https://stackoverflow.com/questions/5019436/python-how-to-terminate-a-blocking-thread
        ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(self._task.ident),
                                                   ctypes.py_object(KeyboardInterrupt))
        time.sleep(0.1)

    def execute_async(self, code_str, filename=''):
        """
        Execute the given code string on a separate thread. This function
        returns as soon as the new thread starts

        :param code_str: A string containing code to execute
        :param filename: See PythonCodeExecution.execute()
        :returns: The created async task
        """
        # Stack is chopped on error to avoid the  AsyncTask.run->self.execute calls appearing
        # as these are not useful for the user in this context
        task = AsyncTask(self.execute, args=(code_str, filename),
                         stack_chop=2,
                         success_cb=self._on_success, error_cb=self._on_error)
        task.start()
        self._task = task
        return task

    def execute(self, code_str, filename=''):
        """Execute the given code on the calling thread
        within the provided context.

        :param code_str: A string containing code to execute
        :param filename: An optional identifier specifying the file source of the code. If None then '<string>'
        is used
        :raises: Any error that the code generates
        """
        filename = '<string>' if filename is None else filename
        compile(code_str, filename, mode='exec')

        sig_progress = self.sig_exec_progress
        for block in code_blocks(code_str):
            sig_progress.emit(block.lineno)
            # compile so we can set the filename
            code_obj = compile(block.code_str, filename, mode='exec')
            exec(code_obj, self.globals_ns, self.locals_ns)

    def reset_context(self):
        # create new context for execution
        self._globals_ns, self._locals_ns = {}, {}

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
            isp.push('\n'*lineno_cur)
