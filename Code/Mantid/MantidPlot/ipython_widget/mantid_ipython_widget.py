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

        import mantidplot
        kernel.shell.push(mantidplot.__dict__)
        import mantid.simpleapi
        kernel.shell.push(mantid.simpleapi.__dict__)
    
        kernel_client = kernel_manager.client()
        kernel_client.start_channels()
    
        self.kernel_manager = kernel_manager
        self.kernel_client = kernel_client

