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

import os
import site

from .pyversion import check_python_version
check_python_version()


def apiVersion():
    """Indicates that this is version 2
    of the API
    """
    return 2


# Bail out early if a Mantid.properties files is not found in the
# parent directory - it indicates a broken installation or build.
_moduledir = os.path.abspath(os.path.dirname(__file__))
_bindir = os.path.dirname(_moduledir)
if not os.path.exists(os.path.join(_bindir, 'Mantid.properties')):
    raise ImportError("Unable to find Mantid.properties file next to this package - broken installation!")

# Make sure the config service loads this properties file
os.environ['MANTIDPATH'] = _bindir
# Add directory as a site directory to process the .pth files
site.addsitedir(_bindir)


try:
    # Flag indicating whether mantidplot layer is loaded.
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
