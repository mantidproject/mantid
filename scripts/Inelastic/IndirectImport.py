# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Temporary solutions to the messy problem of importing F2Py libraries into
the Indirect scripts depending on platform and numpy package version.

We also deal with importing the mantidplot module outside of MantidPlot here.
"""

from __future__ import (absolute_import, division, print_function)
import numpy.core.setup_common as numpy_cfg
import platform
import sys
from mantid import logger

# This is the version of the numpy ABI used when compiling
# the fortran modules with f2py. It must match the version of
# the numpy ABI at runtime.
F2PY_MODULES_REQUIRED_C_ABI = 0x01000009


def import_mantidplot():
    """
    Currently, all scripts in the PythonAlgorithms directory are imported
    during system tests.  Unfortunately, these tests are run outside of
    MantidPlot and so are incompatible with scripts that import the
    "mantidplot" module.  As a result, an error message is dumped to the
    results log for each PythonAlgorithm in the directory that imports
    mantidplot, for each and every test.

    Here, we silently catch all ImportErrors so that this does not occur.

    @returns the mantidplot module.
    """
    try:
        import mantidplot
        return mantidplot
    except ImportError:
        # Not a problem since we are only in a system test anyway, and these
        # scripts are not needed there.
        return None


def _os_env():
    return platform.system() + platform.architecture()[0]


def _lib_suffix():
    if platform.system() == "Windows":
        suffix = "win"
    elif platform.system() == "Linux":
        suffix = "lnx"
    else:
        return ""
    return "_" + suffix + platform.architecture()[0][0:2]


def _numpy_abi_ver():
    """
    Gets the ABI version of Numpy installed on the host system.

    @return The C ABI version
    """
    return numpy_cfg.C_ABI_VERSION


def unsupported_message():
    logger.error('F2Py functionality not currently available on your platform.')
    sys.exit()


def is_supported_f2py_platform():
    """
    We check for numpy version, as if Linux we check its distro and version
    as well.

    @returns True if we are currently on a platform that supports the F2Py
    libraries, else False.
    """
    if (_os_env().startswith("Windows") and
            _numpy_abi_ver() == F2PY_MODULES_REQUIRED_C_ABI and
            "python_d" not in sys.executable):
        return True
    return False


def import_f2py(lib_base_name):
    """
    Until we can include the compilation process of Indirect F2Py modules
    into the automated build of Mantid, we are forced to compile the libraries
    separately on every platform we wish to support.

    Here, we provide a centralised method through which we can import these
    modules, which hopefully makes the other Indirect scripts a lot less messy.

    @param lib_base_name :: is the prefix of the library name.  For example,
    the QLres_lnx64.so and QLres_win32.pyd libraries share the same base name
    of "QLres".

    @returns the imported module.
    """
    # Only proceed if we are indeed on one of the supported platforms.
    assert is_supported_f2py_platform()

    lib_name = lib_base_name + _lib_suffix()

    return __import__(lib_name)


def run_f2py_compatibility_test():
    """
    Convenience method that raises an exception should a user try to run
    the F2Py libraries on an incompatible platform.
    """
    if not is_supported_f2py_platform():
        raise RuntimeError("F2Py programs NOT available on this platform.")
