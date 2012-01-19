import sys

from PyQt4.QtCore import Qt, SIGNAL
from PyQt4.QtGui import QAction, QIcon, QMessageBox, QApplication

# Most IPython modules require `sys.argv` to be defined.
if not hasattr(sys, 'argv'):
    sys.argv = QApplication.instance().argv()

#import resources # Initialize Qt resources from file resources.py

IPYTHON_LOADED = False
import IPython, pygments, matplotlib
from internal_ipkernel import InternalIPKernel
IPYTHON_LOADED = True

class MantidPlot_IPython(object):

    def __init__(self):
        self.kernel = None

    def init_kernel(self):
        kernel = InternalIPKernel()
        kernel.app = QApplication.instance()

        kernel.init_ipkernel('qt')
        # Start the threads that make up the IPython kernel and integrate them
        # with the Qt event loop.
        #kernel.ipkernel.start()

        self.kernel = kernel

    def launch_console(self):
        # If IPython failed to load, bail out.
        if not IPYTHON_LOADED:
            print "Unable to load IPython"
            return

        if self.kernel is None:
            self.init_kernel()

        self.kernel.new_qt_console()
        # Start the threads that make up the IPython kernel and integrate them
        # with the Qt event loop.
        self.kernel.ipkernel.start()
