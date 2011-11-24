"""
api
===

Defines Python objects that wrap the C++ API namespace.

"""

###############################################################################
# The _api C extension depends on exports defined in the _kernel extension
###############################################################################
# Relative imports allow the path to remain unaltered but these are not 
# available in Python 2.4
import sys as _sys
if _sys.version_info[0] == 2 and _sys.version_info[1] > 4:
    # Just use relative imports
    from ..kernel import dlopen as _dlopen
    flags = _dlopen.setup_dlopen() # Ensure the library is open with the correct flags
    from ..kernel import _kernel
    from _api import *
    _dlopen.restore_flags(flags)
else:
    # This is not ideal but little other option for 2.4
    # We also hide the warnings from boost python that are harmless
    import os as _os
    import warnings as _warnings
    _moduledir = _os.path.dirname(__file__)
    # In order for _kernel.DataItem to be properly recognised
    # as a base class in other modules it must be imported 
    # with a fully qualified path, i.e. mantid.kernel._kernel.DataItem
    # hence the rather odd import style here
    _sys.path.append(_os.path.join(_moduledir, '../../'))
    with _warnings.catch_warnings():
        _warnings.simplefilter("ignore")
        from mantid.kernel import dlopen as _dlopen
    flags = _dlopen.setup_dlopen() # Ensure the library is open with the correct flags
    from mantid.kernel import _kernel
    from _api import *
    _dlopen.restore_flags(flags)
    _sys.path.pop()

###############################################################################
# Make the singleton objects available as named variables 
###############################################################################
framework_mgr = get_framework_mgr() # This starts the framework
algorithm_mgr = get_algorithm_mgr()
algorithm_factory = get_algorithm_factory() 
analysis_data_svc = get_analysis_data_service()