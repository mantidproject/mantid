# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Temporary solutions to the messy problem of importing F2Py libraries into
the Indirect scripts depending on platform and numpy package version.

We also deal with importing the mantidplot module outside of MantidPlot here.
"""
from contextlib import contextmanager
import numpy.core.setup_common as numpy_cfg
import platform
import os
import importlib
import sys
from mantid import logger

# This is the version of the numpy ABI used when compiling
# the fortran modules with f2py. It must match the version of
# the numpy ABI at runtime.
F2PY_MODULES_REQUIRED_C_ABI = 0x01000009
BAYES_PACKAGE_NAME = 'quasielasticbayes'
UNSUPPORTED_PLATFORM_MESSAGE = """Functionality not currently available on your platform.
Please try installing the extra package: python -m pip install --user quasielasticbayes
"""


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


def is_pip_version_of_libs():
    """
    If we are using a pip version of the libs we can import them from the BAYES_PACKAGE_NAME package
    import BAYES_PACKAGE_NAME
    # imports BAYES_PACKAGE_NAME.QLdata ect., i.e. the libraries are usable with:
    BAYES_PACKAGE_NAME.QLdata.qldata(....)
    ...
    Which will import the binaries from the python path,
    most likely the site-packages folder
    """
    if importlib.util.find_spec(BAYES_PACKAGE_NAME) is not None:
        return True, BAYES_PACKAGE_NAME + '.'
    else:
        return False, ""


def _lib_suffix():
    """
    If we are using a pip version of the libs they are NOT suffixed.
    They are imported with
    import Quest
    import QLdata
    ..
    If we are using the internally shipped libraries, we HAVE a suffix
    This means we import the quest library with:
    import Quest_win64
    import QLdata_win64
    Thefore, if we are using the pip versions we don't add a suffix.
    """
    is_pip, _ = is_pip_version_of_libs()
    if is_pip:
        return ""
    else:
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
    logger.error(UNSUPPORTED_PLATFORM_MESSAGE)
    sys.exit()


def is_supported_f2py_platform():
    """
    We check for numpy version, as if Linux we check its distro and version
    as well.

    @returns True if we are currently on a platform that supports the F2Py
    libraries, else False.
    """
    if (_os_env().startswith("Windows") and "CONDA_PREFIX" not in os.environ
            and _numpy_abi_ver() == F2PY_MODULES_REQUIRED_C_ABI
            and "python_d" not in sys.executable):
        return True
    # check if we have pip installed the fortran libraries
    # first check the numpy abi is correct
    if _numpy_abi_ver() == F2PY_MODULES_REQUIRED_C_ABI:
        return is_pip_version_of_libs()[0]
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

    @contextmanager
    def in_syspath(directory):
        sys.path.insert(0, directory)
        yield
        sys.path.pop(0)
    _, package_base = is_pip_version_of_libs()
    lib_name = package_base + lib_base_name + _lib_suffix()
    return __import__(lib_name, fromlist=[None])


def run_f2py_compatibility_test():
    """
    Convenience method that raises an exception should a user try to run
    the F2Py libraries on an incompatible platform.
    """
    if not is_supported_f2py_platform():
        raise RuntimeError(UNSUPPORTED_PLATFORM_MESSAGE)
