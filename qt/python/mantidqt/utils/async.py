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
import time
import traceback


def blocking_async_task(target, args=(), kwargs=None, blocking_cb=None,
                        period_secs=0.05):
    """Run the target in a separate thread and block the calling thread
    until execution is complete.

    :param target: A Python callable object
    :param args: Arguments to pass to the callable
    :param kwargs: Keyword arguments to pass to the callable
    :param blocking_cb: An optional callback to process while waiting for the task
    to finish
    :param period_secs: Sleep for this many seconds at the start of each loop that checks
    the task is still alive. This will be the minimum time between calls to blocking_cb.
    :returns: An AsyncTaskResult object
    """
    blocking_cb = blocking_cb if blocking_cb is not None else lambda: None

    class Receiver(object):
        output, exception = None, None

        def on_success(self, result):
            self.output = result.output

        def on_error(self, result):
            self.exception = result.exception

    recv = Receiver()
    task = AsyncTask(target, args, kwargs, success_cb=recv.on_success,
                     error_cb=recv.on_error)
    task.start()
    while task.is_alive():
        time.sleep(period_secs)
        blocking_cb()

    if recv.exception is not None:
        raise recv.exception
    else:
        return recv.output


class AsyncTask(threading.Thread):

    def __init__(self, target, args=(), kwargs=None,
                 stack_chop=0,
                 success_cb=None, error_cb=None,
                 finished_cb=None):
        """

        :param target: A Python callable object
        :param args: Arguments to pass to the callable
        :param stack_chop: If an error is raised then chop this many entries
        from the top of traceback stack.
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
        self.stack_chop = stack_chop

        self.success_cb = success_cb if success_cb is not None else lambda x: None
        self.error_cb = error_cb if error_cb is not None else lambda x: None
        self.finished_cb = finished_cb if finished_cb is not None else lambda: None

    def run(self):
        try:
            out = self.target(*self.args, **self.kwargs)
        except SyntaxError as exc:
            # treat SyntaxErrors as special as the traceback makes no sense
            # and the lineno is part of the exception instance
            self.error_cb(AsyncTaskFailure(exc, None))
        except:  # noqa
            self.error_cb(AsyncTaskFailure.from_excinfo(self.stack_chop))
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

    @staticmethod
    def from_excinfo(chop=0):
        """
        Create an AsyncTaskFailure from the current exception info

        :param chop: Trim this number of entries from
        the top of the stack listing
        :return: A new AsyncTaskFailure object
        """
        _, exc_value, exc_tb = sys.exc_info()
        return AsyncTaskFailure(exc_value, traceback.extract_tb(exc_tb)[chop:])

    def __init__(self, exception, stack_entries):
        self.exception = exception
        self.stack_entries = stack_entries

    @property
    def success(self):
        return False
