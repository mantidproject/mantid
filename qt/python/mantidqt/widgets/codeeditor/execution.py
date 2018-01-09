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

# local imports
from mantidqt.utils.async import AsyncTask


class PythonCodeExecution(object):
    """Provides the ability to execute arbitrary
    strings of Python code. It supports
    reporting progress updates in asynchronous execution
    """
    on_success = None
    on_error = None
    on_progress = None

    def __init__(self,success_cb=None, error_cb=None,
                 progress_cb=None):
        """
        Initialize the object

        :param success_cb: A callback of the form f() called on success
        :param error_cb: A callback of the form f(exc) called on error,
        providing an AsyncTaskFailure object
        :param progress_cb: A callback for progress updates as the code executes
        """
        # AsyncTask's callbacks have a slightly different form
        if success_cb:
            def success_cb_wrap(_): success_cb()
        else:
            def success_cb_wrap(_): pass
        self.on_success = success_cb_wrap
        self.on_error = error_cb

        self.on_progress_update = progress_cb

    def execute_async(self, code_str, user_globals,
                      user_locals):
        """
        Execute the given code string on a separate thread. This function
        returns as soon as the new thread starts

        :param code_str: A string containing code to execute
        :param user_globals: A mutable mapping type to store global variables
        :param user_locals: A mutable mapping type to store local variables
        :returns: The created async task
        """
        # Stack is chopped on error to avoid the  AsyncTask.run->_do_exec->exec calls appearing
        # as these are not useful in this context
        t = AsyncTask(self.execute, args=(code_str, user_globals, user_locals),
                      stack_chop=3,
                      success_cb=self.on_success, error_cb=self.on_error)
        t.start()
        return t

    def execute(self, code_str, user_globals,
                user_locals):
        """Execute the given code on the calling thread
        within the provided context.

        :param code_str: A string containing code to execute
        :param user_globals: A mutable mapping type to store global variables
        :param user_locals: A mutable mapping type to store local variables
        :raises: Any error that the code generates
        """
        # execute whole string if no reporting is required
        if self.on_progress_update is None:
            self._do_exec(code_str, user_globals, user_locals)
        else:
            self._execute_as_blocks(code_str, user_globals, user_locals,
                                    self.on_progress_update)

    def _execute_as_blocks(self, code_str, user_globals, user_locals,
                           progress_cb):
        """Execute the code in the supplied context and report the progress
        using the supplied callback"""
        # will raise a SyntaxError if any of the code is invalid
        compile(code_str, "<string>", mode='exec')

        for block in code_blocks(code_str):
            progress_cb(block.lineno)
            self._do_exec(block.code_obj, user_globals, user_locals)

    def _do_exec(self, code, user_globals, user_locals):
        exec (code, user_globals, user_locals)


class CodeBlock(object):
    """Holds an executable code object. It also stores the line number
    of the first line within a larger group of code blocks"""

    def __init__(self, code_obj, lineno):
        self.code_obj = code_obj
        self.lineno = lineno


def code_blocks(code_str):
    """Generator to produce blocks of executable code
    from the given code string.
    """
    lineno = 1
    lines = code_str.splitlines()
    cur_block = []
    for line in lines:
        cur_block.append(line)
        code_block = "\n".join(cur_block)
        try:
            code_obj = compile(code_block, "<string>", mode='exec')
            yield CodeBlock(code_obj, lineno)
            lineno += len(cur_block)
            cur_block = []
        except (SyntaxError, TypeError):
            # assume we don't have a full block yet
            continue
