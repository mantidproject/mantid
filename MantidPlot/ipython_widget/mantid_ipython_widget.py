import threading
import types
import inspect

from PyQt4 import QtGui

# IPython monkey patches the  pygments.lexer.RegexLexer.get_tokens_unprocessed method
# and breaks Sphinx when running within MantidPlot.
# We store the original method definition here on the pygments module before importing IPython
from pygments.lexer import RegexLexer
# Monkeypatch!
RegexLexer.get_tokens_unprocessed_unpatched = RegexLexer.get_tokens_unprocessed

from IPython.qt.console.rich_ipython_widget import RichIPythonWidget
from IPython.qt.inprocess import QtInProcessKernelManager


def our_run_code(self, code_obj, result=None):
    """ Method with which we replace the run_code method of IPython's InteractiveShell class.
        It calls the original method (renamed to ipython_run_code) on a separate thread
        so that we can avoid locking up the whole of MantidPlot while a command runs.

        Parameters
        ----------
        code_obj : code object
          A compiled code object, to be executed
        result : ExecutionResult, optional
          An object to store exceptions that occur during execution.
        Returns
        -------
        False : Always, as it doesn't seem to matter.
    """

    t = threading.Thread()
    #ipython 3.0 introduces a third argument named result
    nargs = len(inspect.getargspec(self.ipython_run_code).args)
    if (nargs == 3):
        t = threading.Thread(target=self.ipython_run_code, args=[code_obj,result])
    else:
        t = threading.Thread(target=self.ipython_run_code, args=[code_obj])
    t.start()
    while t.is_alive():
        QtGui.QApplication.processEvents()
    # We don't capture the return value of the ipython_run_code method but as far as I can tell
    #   it doesn't make any difference what's returned
    return 0


class MantidIPythonWidget(RichIPythonWidget):
    """ Extends IPython's qt widget to include setting up and in-process kernel as well as the
        Mantid environment, plus our trick to avoid blocking the event loop while processing commands.
        This widget is set in the QDockWidget that houses the script interpreter within ApplicationWindow.
    """

    def __init__(self, *args, **kw):
        super(MantidIPythonWidget, self).__init__(*args, **kw)

        # Create an in-process kernel
        kernel_manager = QtInProcessKernelManager()
        kernel_manager.start_kernel()
        kernel = kernel_manager.kernel
        kernel.gui = 'qt4'

        # Figure out the full path to the mantidplotrc.py file and then %run it
        from os import path
        mantidplotpath = path.split(path.dirname(__file__))[0] # It's the directory above this one
        mantidplotrc = path.join(mantidplotpath, 'mantidplotrc.py')
        shell = kernel.shell
        shell.run_line_magic('run',mantidplotrc)

        # These 3 lines replace the run_code method of IPython's InteractiveShell class (of which the
        # shell variable is a derived instance) with our method defined above. The original method
        # is renamed so that we can call it from within the our_run_code method.
        f = shell.run_code
        shell.run_code = types.MethodType(our_run_code, shell)
        shell.ipython_run_code = f

        kernel_client = kernel_manager.client()
        kernel_client.start_channels()

        self.kernel_manager = kernel_manager
        self.kernel_client = kernel_client

