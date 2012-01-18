"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
# The fully-qualified package path allows it to be found with path manipulation
from mantid.kernel import dlopen as _dlopen
import os as _os
clib = _os.path.join(_os.path.dirname(__file__), '_api.so')
flags = _dlopen.setup_dlopen(clib, ['libMantidKernel', 'libMantidGeometry', 'libMantidAPI']) # Ensure the library is open with the correct flags
from mantid.kernel import _kernel
from _api import *
_dlopen.restore_flags(flags)

###############################################################################
# Attach operators to workspaces 
###############################################################################
import workspaceops as _ops
_ops.add_operators_to_workspace()

###############################################################################
# Make the singleton objects available as named variables 
###############################################################################
FrameworkManager.Instance() # This starts the framework

###############################################################################
# Starting the FrameworkManager loads the C++ plugin libraries
# we need to load in the Python plugins as well
###############################################################################
import mantid.kernel.plugins as _plugins
# Algorithms
from mantid.kernel import config as _cfg
# Disabled for the time being as all algorithms are of the old kind
#_plugins.load(_cfg['pythonalgorithm.directories']) 


###############################################################################
# When in GUI mode we want to be picky about algorithm execution as we
# currently can't run the scripts in a separate thread:
#
#   Full managed algorithms called from a script - asynchronous
#   Child algorithms called from with Python algorithms - synchronous
# So that users can just all alg.execute() and have it do the right thing we will
# replace the default method with the selective one here
###############################################################################
import mantid
if mantid.__gui__ == True:
    # Save the original function
    _orig_execute = IAlgorithm.execute
    import _qti
    def execute_wrapper(self):
        if hasattr(self, '__async__') and self.__async__:
            success = _qti.app.mantidUI.runAlgorithmAsync_PyCallback(self.name())
            if not success:
                raise RuntimeError('An error occurred while running %s. See results log for details.' % self.name())
        else:
            _orig_execute(self)
    
    # Replace the attribute
    setattr(IAlgorithm, 'execute', execute_wrapper)
