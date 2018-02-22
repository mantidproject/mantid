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
import inspect
import time

# 3rdparty imports
from qtpy.QtCore import QObject, Signal
from six import PY2, iteritems

# local imports
from mantidqt.widgets.codeeditor.inputsplitter import InputSplitter
from mantidqt.utils.async import AsyncTask

if PY2:
    from inspect import getargspec as getfullargspec
else:
    from inspect import getfullargspec


def get_function_spec(func):
    """Get the python function signature for the given function object. First
    the args are inspected followed by varargs, which are set by some modules,
    e.g. mantid.simpleapi algorithm functions

    :param func: A Python function object
    :returns: A string containing the function specification
    :
    """
    try:
        argspec = getfullargspec(func)
    except TypeError:
        return ''
    # mantid algorithm functions have varargs set not args
    args = argspec[0]
    if args:
        # For methods strip the self argument
        if hasattr(func, 'im_func'):
            args = args[1:]
        defs = argspec[3]
    elif argspec[1] is not None:
        # Get from varargs/keywords
        arg_str = argspec[1].strip().lstrip('\b')
        defs = []
        # Keyword args
        kwargs = argspec[2]
        if kwargs is not None:
            kwargs = kwargs.strip().lstrip('\b\b')
            if kwargs == 'kwargs':
                kwargs = '**' + kwargs + '=None'
            arg_str += ',%s' % kwargs
        # Any default argument appears in the string
        # on the rhs of an equal
        for arg in arg_str.split(','):
            arg = arg.strip()
            if '=' in arg:
                arg_token = arg.split('=')
                args.append(arg_token[0])
                defs.append(arg_token[1])
            else:
                args.append(arg)
        if len(defs) == 0:
            defs = None
    else:
        return ''

    if defs is None:
        calltip = ','.join(args)
        calltip = '(' + calltip + ')'
    else:
        # The defaults list contains the default values for the last n arguments
        diff = len(args) - len(defs)
        calltip = ''
        for index in range(len(args) - 1, -1, -1):
            def_index = index - diff
            if def_index >= 0:
                calltip = '[' + args[index] + '],' + calltip
            else:
                calltip = args[index] + "," + calltip
        calltip = '(' + calltip.rstrip(',') + ')'
    return calltip


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

        if startup_code:
            self.execute(startup_code)

    @property
    def globals_ns(self):
        return self._globals_ns

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

    def execute(self, code_str, filename=None):
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
            exec(code_obj, self.globals_ns, self.globals_ns)

    def generate_calltips(self):
        """
        Return a list of calltips for the current global scope. This is currently
        very basic and only inspects the available functions and builtins at the current scope.

        :return: A list of strings giving calltips for each global callable
        """
        calltips = []
        for name, attr in iteritems(self._globals_ns):
            if inspect.isfunction(attr) or inspect.isbuiltin(attr):
                calltips.append(name + get_function_spec(attr))

        return calltips

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
