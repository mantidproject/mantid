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

    def execute_async(self, code_str, user_globals,
                      user_locals, success_cb=None, error_cb=None,
                      progress_cb=None):
        """
        Execute the given code string on a separate thread. This function
        returns as soon as the new thread starts

        :param code_str: A string containing code to execute
        :param user_globals: A mutable mapping type to store global variables
        :param user_locals: A mutable mapping type to store local variables
        :param success_cb: A callback of the form f() called on success
        :param error_cb: A callback of the form f(exc) called on error,
        :param progress_cb: A callback for progress updates
        providing the exception generated
        :returns: The created async task
        """
        # AsyncTask's callbacks have a slightly different form
        if success_cb:
            def on_success(_): success_cb()
        else:
            def on_success(_): pass

        if error_cb:
            def on_error(task_result): error_cb(task_result.exception)
        else:
            def on_error(_): pass

        t = AsyncTask(self.execute, args=(code_str, user_globals, user_locals),
                      success_cb=on_success, error_cb=on_error)
        t.start()
        return t

    def execute(self, code_str, user_globals,
                user_locals):
        """Execute the given code on the calling thread
        within the provided context. All exceptions are caught
        and stored with the returned result

        :param code_str: A string containing code to execute
        :param user_globals: A mutable mapping type to store global variables
        :param user_locals: A mutable mapping type to store local variables
        :raises: Any error that the code generates
        """
        exec(code_str, user_globals, user_locals)
