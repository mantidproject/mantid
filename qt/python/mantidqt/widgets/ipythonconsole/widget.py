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
from __future__ import absolute_import

# system imports
import inspect
import types

# 3rd party imports
# qtpy must be the first import here as it makes the selection of the PyQt backend
# by preferring PyQt5 as we would like
from qtpy.QtWidgets import QApplication
try:
    # Later versions of Qtconsole are part of Jupyter
    from qtconsole.rich_jupyter_widget import RichJupyterWidget as RichIPythonWidget
    from qtconsole.inprocess import QtInProcessKernelManager
except ImportError:
    from IPython.qt.console.rich_ipython_widget import RichIPythonWidget
    from IPython.qt.inprocess import QtInProcessKernelManager

# local imports
from mantidqt.utils.async import blocking_async_task


class InProcessIPythonConsole(RichIPythonWidget):

    def __init__(self, *args, **kwargs):
        """
        A constructor matching that of RichIPythonWidget
        :param args: Positional arguments passed directly to RichIPythonWidget
        :param kwargs: Keyword arguments. The following are used by this
        widget:
          - banner_extra: An additinal string to append to the default banner
        """
        # remove our arguments
        banner_extra = kwargs.pop("banner_extra", "")
        super(InProcessIPythonConsole, self).__init__(*args, **kwargs)

        # create an in-process kernel
        kernel_manager = QtInProcessKernelManager()
        kernel_manager.start_kernel()
        kernel = kernel_manager.kernel
        kernel.gui = 'qt'

        # use a separate thread for execution
        shell = kernel.shell
        shell.run_code = async_wrapper(shell.run_code, shell)

        # custom banner
        if banner_extra:
            self.banner += "\n" + banner_extra

        # attach channels and start kenel
        kernel_client = kernel_manager.client()
        kernel_client.start_channels()

        self.kernel_manager = kernel_manager
        self.kernel_client = kernel_client


def async_wrapper(orig_run_code, shell_instance):
    """
    Return a new method that wraps the original and runs it asynchronously
    :param orig_run_code: The original run_code bound method
    :param shell_instance: The shell instance associated with the orig_run_code
    :return: A new method that can be attached to shell_instance
    """
    def async_run_code(self, code_obj, result=None):
        """A monkey-patched replacement for the InteractiveShell.run_code method.
        It runs the in a separate thread and calls QApplication.processEvents
        periodically until the method finishes
        """
        # ipython 3.0 introduces a third argument named result
        if len(inspect.getargspec(orig_run_code).args) == 3:
            args = (code_obj, result)
        else:
            args = (code_obj,)
        return blocking_async_task(target=orig_run_code, args=args,
                                   blocking_cb=QApplication.processEvents)

    return types.MethodType(async_run_code, shell_instance)
