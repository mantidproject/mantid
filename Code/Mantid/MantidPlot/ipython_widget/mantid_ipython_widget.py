from IPython.qt.console.rich_ipython_widget import RichIPythonWidget
from IPython.qt.inprocess import QtInProcessKernelManager

class MantidIPythonWidget(RichIPythonWidget):

    def __init__(self, *args, **kw):
        super(MantidIPythonWidget, self).__init__(*args, **kw)

        # Create an in-process kernel
        kernel_manager = QtInProcessKernelManager()
        kernel_manager.start_kernel()
        kernel = kernel_manager.kernel
        kernel.gui = 'qt4'

        # Figure out the full path to the mantidplotrc.py file and then %run it
        from os import path
        mantidplotpath = path.split(path.dirname(__file__))[0]
        if not mantidplotpath:
            # If the the file is in the cwd, then path.dirname returns an empty string
            mantidplotrc = 'mantidplotrc.py'
        else:
            mantidplotrc = mantidplotpath + '/mantidplotrc.py'
        kernel.shell.run_line_magic('run',mantidplotrc)
    
        kernel_client = kernel_manager.client()
        kernel_client.start_channels()
    
        self.kernel_manager = kernel_manager
        self.kernel_client = kernel_client

