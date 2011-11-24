"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
from ..kernel import dlopen as _dlopen
flags = _dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from ..kernel import _kernel
from _api import *
_dlopen.restore_flags(flags)

###############################################################################
# Make the singleton objects available as named variables 
###############################################################################
framework_mgr = get_framework_mgr() # This starts the framework
algorithm_mgr = get_algorithm_mgr()
algorithm_factory = get_algorithm_factory() 
analysis_data_svc = get_analysis_data_service()