"""Defines the api sub-package
"""
import sys

from kernel import dlopen
flags = dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _api import *
dlopen.restore_flags(flags)

# Singleton objects
framework_mgr = get_framework_mgr() # The first import of this starts the framework
algorithm_mgr = get_algorithm_mgr()
algorithm_factory = get_algorithm_factory() 
analysis_data_svc = get_analysis_data_service()


# Control what an import * does
__all__ = dir(_api)
__all__.extend(['framework_mgr','algorithm_mgr','algorithm_factory','analysis_data_svc'])
