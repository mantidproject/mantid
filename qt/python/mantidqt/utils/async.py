#  This file is part of the mantid workbench.
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
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import

# system imports
import sys
import threading


class AsyncTask(threading.Thread):

    def __init__(self, target, args=(), kwargs=None,
                 success_cb=None, error_cb=None,
                 finished_cb=None):
        """

        :param target: A Python callable object
        :param args: Arguments to pass to the callable
        :param kwargs: Keyword arguments to pass to the callable
        :param success_cb: Optional callback called when operation was successful
        :param error_cb: Optional callback called when operation was not successful
        :param finished_cb: Optional callback called when operation has finished
        """
        super(AsyncTask, self).__init__()
        if not callable(target):
            raise TypeError("Target object is required to be callable")

        self.target = target
        self.args = args
        self.kwargs = kwargs if kwargs is not None else {}
        self.success_cb = success_cb if success_cb is not None else lambda x: None
        self.error_cb = error_cb if error_cb is not None else lambda x: None
        self.finished_cb = finished_cb if finished_cb is not None else lambda: None

    def run(self):
        try:
            out = self.target(*self.args, **self.kwargs)
        except:  # noqa
            self.error_cb(AsyncTaskFailure(sys.exc_info()[1]))
        else:
            self.success_cb(AsyncTaskSuccess(out))

        self.finished_cb()


class AsyncTaskSuccess(object):
    """Object describing the successful execution of an asynchronous task
    """

    def __init__(self, output):
        self.output = output

    @property
    def success(self):
        return True


class AsyncTaskFailure(object):
    """Object describing the failed execution of an asynchronous task
    """

    def __init__(self, exception):
        self.exception = exception

    @property
    def success(self):
        return False
