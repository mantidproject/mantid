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
import importlib

BAYES_PACKAGE_NAME = 'quasielasticbayes'
UNSUPPORTED_PLATFORM_MESSAGE = """Functionality not currently available on your platform.
Please try installing the extra package: python -m pip install --user quasielasticbayes
"""


def unsupported_message():
    raise ImportError(UNSUPPORTED_PLATFORM_MESSAGE)


def is_supported_f2py_platform():
    """
    @returns True if we are currently on a platform that supports the F2Py
    libraries, else False.
    """
    return importlib.util.find_spec(BAYES_PACKAGE_NAME) is not None


def import_f2py(lib_base_name):
    """
    Until we can include the compilation process of Indirect F2Py modules
    into the automated build of Mantid, we are forced to compile the libraries
    separately on every platform we wish to support.

    Here, we provide a centralised method through which we can import these
    modules, which hopefully makes the other Indirect scripts a lot less messy.

    @param lib_base_name :: is the prefix of the library name.  For example,
    the QLres.

    @returns the imported module.
    """
    # Only proceed if we are indeed on one of the supported platforms.
    assert is_supported_f2py_platform()

    return __import__(f"{BAYES_PACKAGE_NAME}.{lib_base_name}", fromlist=[None])


def run_f2py_compatibility_test():
    """
    Convenience method that raises an exception should a user try to run
    the F2Py libraries on an incompatible platform.
    """
    if not is_supported_f2py_platform():
        raise RuntimeError(UNSUPPORTED_PLATFORM_MESSAGE)
