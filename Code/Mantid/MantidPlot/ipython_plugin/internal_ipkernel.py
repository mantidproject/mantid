"""
This code is a modified vesion of an IPython example created by Fernando Perez
for the development version of IPython v0.12:

  https://github.com/ipython/ipython/blob/4e1a76c/docs/examples/lib/internal_ipkernel.py
"""
#-----------------------------------------------------------------------------
# Imports
#-----------------------------------------------------------------------------

from IPython.lib.kernel import connect_qtconsole, get_connection_file
from IPython.zmq.ipkernel import IPKernelApp

#-----------------------------------------------------------------------------
# Functions and classes
#-----------------------------------------------------------------------------

def pylab_kernel(gui):
    """Launch and return an IPython kernel with pylab support for the desired gui
    """
    kernel = IPKernelApp.instance()
    # Note: pylab command seems to be needed for event loop to behave nicely
    kernel.initialize(['python', '--pylab=%s' % gui,
        "--c='%run -m mantidplotrc'"])
    return kernel


class InternalIPKernel(object):

    def init_ipkernel(self, backend):
        # Start IPython kernel with GUI event loop and pylab support
        self.ipkernel = pylab_kernel(backend)
        # To create and track active qt consoles
        self.consoles = []

        # This application will also act on the shell user namespace
        self.namespace = self.ipkernel.shell.user_ns
        # Keys present at startup so we don't print the entire pylab/numpy
        # namespace when the user clicks the 'namespace' button
        self._init_keys = set(self.namespace.keys())

        # Example: a variable that will be seen by the user in the shell, and
        # that the GUI modifies (the 'Counter++' button increments it):
        self.namespace['app_counter'] = 0
        #self.namespace['ipkernel'] = self.ipkernel  # dbg

    def new_qt_console(self, evt=None):
        """start a new qtconsole connected to our kernel"""
        
        import sys
        # We have to step in and cannibalise connect_qtconsole if we're on windows because
        # it launches sys.executable assuming it'll be python, when in fact it's MantidPlot
        if sys.platform == 'win32':
            argv = []
            cf = get_connection_file()
            cmd = ';'.join([
                "from IPython.frontend.qt.console import qtconsoleapp",
                "qtconsoleapp.main()"
            ])
            from subprocess import Popen, PIPE
            return Popen([sys.exec_prefix+'\pythonw.exe', '-c', cmd, '--existing', cf] + argv, stdout=PIPE, stderr=PIPE)
        
        return connect_qtconsole()

    def cleanup_consoles(self, evt=None):
        for c in self.consoles:
            c.kill()
