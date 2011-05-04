#!/usr/bin/env python
"""
  Python install script for Mantid. Assumes that Mantid has been properly compiled.
  Users will need to have their environment variables set properly:

    MANTIDPATH should be set to the Mantid release directory
    (DY)LD_LIBRARY_PATH should also contain the Mantid release directory
"""
from distutils.core import setup
#import os

# Python module for Mantid has a different name on Windows
#if os.name == 'nt':
#    python_lib = "MantidPythonAPI.pyd"
#else:
#    python_lib = "libMantidPythonAPI.so"

setup(name='Mantid',
      version='1.0',
      description='Python bindings for Mantid',
      url='http://www.mantidproject.org',
      packages=['MantidFramework'],
      package_dir={'MantidFramework' : ''},
     )
