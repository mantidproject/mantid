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

def get_executable():
    """Returns a string giving the executable to run when starting the IPython kernel
    """
    # We have to make sure we can actually call the Python executable as this script will
    # have probably been called from MantidPlot and in that instance MantidPlot will
    # be the sys.executable file. Assume that Linux & OS X can just call Python but
    # on Windows we will have to assume that python.exe is next to MantidPlot.exe
    # but not necessarily in the PATH variable
    import sys
    if sys.platform == "win32":
        import os
        # pythonw avoids a new terminal window when the new process starts (only applicable
        # on the packaged build
        return os.path.join(sys.exec_prefix, "pythonw.exe")
    else:
        return "python"

def pylab_kernel(gui):
    """Launch and return an IPython kernel with pylab support for the desired gui
    """
    kernel = IPKernelApp.instance()
    # Note: pylab command seems to be needed for event loop to behave nicely
    kernel.initialize([get_executable(), '--pylab=%s' % gui,
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
            return Popen([get_executable(), '-c', cmd, '--existing', cf] + argv, stdout=PIPE, stderr=PIPE)
        
        return connect_qtconsole()

    def cleanup_consoles(self, evt=None):
        for c in self.consoles:
            c.kill()
