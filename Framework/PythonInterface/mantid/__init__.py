# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Mantid
======

http://www.mantidproject.org

The Mantid project provides a platform that supports high-performance computing
on neutron and muon data. The framework provides a set of common services,
algorithms and data objects that are:

    - Instrument or technique independent;
    - Supported on multiple target platforms (Windows, Linux, Mac OS X);
    - Easily extensible by Instruments Scientists/Users;
    - Open source and freely redistributable to visiting scientists;
    - Provides functionalities for Scripting, Visualization, Data transformation,
      Implementing Algorithms, Virtual Instrument Geometry.

"""
from __future__ import (absolute_import, division,
                        print_function)

from .pyversion import check_python_version
check_python_version()


def apiVersion():
    """Indicates that this is version 2
    of the API
    """
    return 2

# Flag indicating whether mantidplot layer is loaded.
try:
    import _qti
    __gui__ = True
except ImportError:
    __gui__ = False

# Set deprecation warnings back to default (they are ignored in 2.7)
import warnings as _warnings
# Default we see everything
_warnings.filterwarnings("default",category=DeprecationWarning,
                         module="mantid.*")
# We can't do anything about numpy.oldnumeric being deprecated but
# still used in other libraries, e.g scipy, so just ignore those
_warnings.filterwarnings("ignore",category=DeprecationWarning,
                         module="numpy.oldnumeric")


# Peek to see if a Mantid.properties file is in the parent directory,
# if so assume that it is the required Mantid bin directory containing
# the Mantid libraries and ignore any MANTIDPATH that has been set
import os as _os
_moduledir = _os.path.abspath(_os.path.dirname(__file__))
_bindir = _os.path.dirname(_moduledir)
if _os.path.exists(_os.path.join(_bindir, 'Mantid.properties')):
    _os.environ['MANTIDPATH'] = _bindir

###############################################################################
# Load all subpackages that contain a C-extension. The boost.python
# registry will be missing entries if all are not loaded.
###############################################################################
from . import kernel as _kernel
from . import api as _api
from . import geometry as _geometry
from . import dataobjects as _dataobjects
from . import _plugins

# Make the aliases from each module accessible in a the mantid namespace
from .kernel._aliases import *
from .api._aliases import *

# Make the version string accessible in the standard way
from .kernel import version_str as _version_str
__version__ = _version_str()
