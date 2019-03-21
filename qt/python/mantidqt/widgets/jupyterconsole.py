# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
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
    from qtconsole.rich_jupyter_widget import RichJupyterWidget
    from qtconsole.inprocess import QtInProcessKernelManager
except ImportError:
    from IPython.qt.console.rich_ipython_widget import RichIPythonWidget as RichJupyterWidget
    from IPython.qt.inprocess import QtInProcessKernelManager

# local imports
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback


class InProcessJupyterConsole(RichJupyterWidget):

    def __init__(self, *args, **kwargs):
        """
        A constructor matching that of RichJupyterWidget
        :param args: Positional arguments passed directly to RichJupyterWidget
        :param kwargs: Keyword arguments. The following keywords are understood by this widget:

          - startup_code: A code snippet to run on startup.

        the rest are passed to RichJupyterWidget
        """
        startup_code = kwargs.pop("startup_code", "")
        super(InProcessJupyterConsole, self).__init__(*args, **kwargs)

        # create an in-process kernel
        kernel_manager = QtInProcessKernelManager()
        kernel_manager.start_kernel()
        kernel = kernel_manager.kernel
        kernel.gui = 'qt'

        # use a separate thread for execution
        shell = kernel.shell
        shell.run_code = async_wrapper(shell.run_code, shell)

        # attach channels, start kernel and run any startup code
        kernel_client = kernel_manager.client()
        kernel_client.start_channels()
        if startup_code:
            shell.ex(startup_code)

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
        task = BlockingAsyncTaskWithCallback(target=orig_run_code, args=args, blocking_cb=QApplication.processEvents)
        return task.start()

    return types.MethodType(async_run_code, shell_instance)
