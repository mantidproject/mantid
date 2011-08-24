"""Defines the kernel sub-package
"""
import dlopen
flags = dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _kernel import *
dlopen.restore_flags(flags)