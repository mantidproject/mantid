# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

import os
import sys

import site
from .buildconfig import check_python_version

check_python_version()


def apiVersion():
    """Indicates that this is version 2
    of the API
    """
    return 2


def _bin_dirs():
    """
    Generate a list of possible paths that contain the Mantid.properties file
    """
    _moduledir = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))

    # standard packaged install
    yield _moduledir

    # conda layout
    yield os.path.dirname(sys.executable)

    # conda windows layout
    yield os.path.join(os.path.dirname(sys.executable), "Library", "bin")

    # iterate over the PYTHONPATH, to scan all possible bin dirs
    for path in sys.path:
        yield path


# Bail out early if a Mantid.properties files is not found in
# one of the expected places - it indicates a broken installation or build.
_bindir = None
for path in _bin_dirs():
    if os.path.exists(os.path.join(path, "Mantid.properties")):
        _bindir = path
        break

if _bindir is None:
    raise ImportError(
        "Broken installation! Unable to find Mantid.properties file.\n" "Directories searched: {}".format(", ".join(_bin_dirs()))
    )

# Windows doesn't have rpath settings so make sure the C-extensions can find the rest of the
# mantid dlls. We assume they will be next to the properties file.
if sys.platform == "win32":
    os.environ["PATH"] = _bindir + ";" + os.environ.get("PATH", "")
# Make sure the config service loads this properties file
os.environ["MANTIDPATH"] = _bindir
# Add directory as a site directory to process the .pth files
site.addsitedir(_bindir)

try:
    # Flag indicating whether mantidplot layer is loaded.
    import _qti  # noqa: F401

    __gui__ = True
except ImportError:
    __gui__ = False

# Set deprecation warnings back to default (they are ignored in 2.7)
import warnings as _warnings

# Default we see everything
_warnings.filterwarnings("default", category=DeprecationWarning, module="mantid.*")
# We can't do anything about numpy.oldnumeric being deprecated but
# still used in other libraries, e.g scipy, so just ignore those
_warnings.filterwarnings("ignore", category=DeprecationWarning, module="numpy.oldnumeric")

###############################################################################
# Load all non-plugin subpackages that contain a C-extension. The boost.python
# registry will be missing entries if all are not loaded.
###############################################################################
from mantid import kernel as _kernel  # noqa: F401
from mantid import api as _api  # noqa: F401
from mantid import geometry as _geometry  # noqa: F401
from mantid import dataobjects as _dataobjects  # noqa: F401

# Make the aliases from each module accessible in the mantid namespace
from mantid.kernel._aliases import *
from mantid.api._aliases import *

# Make the version string and info accessible in the standard way
from mantid.kernel import version_str as _version_str
from mantid.kernel import version  # noqa: F401

__version__ = _version_str()
