"""Defines the api sub-package
"""
import sys

from kernel import dlopen
flags = dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _api import *
dlopen.restore_flags(flags)

# Alias the singleton objects
algorithm_mgr = get_algorithm_mgr()
 