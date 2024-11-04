# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: F403   # Allow wild imports
"""cxxtest: A Python package that supports the CxxTest test framework for C/C++.

.. _CxxTest: http://cxxtest.tigris.org/

The _CxxTest testing framework is focussed on being a lightweight
framework that is well suited for integration into embedded systems
development projects.

CxxTest's advantages over existing alternatives are that it:

* Doesn't require RTTI
* Doesn't require member template functions
* Doesn't require exception handling
* Doesn't require any external libraries (including memory management, file/console I/O, graphics libraries)
* Is distributed entirely as a set of header files (and a python script).
* Doesn't require the user to manually register tests and test suites

The cxxtest Python package includes capabilities for parsing C/C++ source files and generating
CxxTest drivers.
"""

from cxxtest.__release__ import __version__, __date__  # noqa: F401

__maintainer__ = "TODO"
__maintainer_email__ = "TODO"
__copyright__ = "TODO"
__license__ = "LGPL"
__url__ = "http://cxxtest.tigris.org/"

from cxxtest.cxxtestgen import *
