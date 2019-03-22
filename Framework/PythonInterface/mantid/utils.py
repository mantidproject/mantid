# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from importlib import import_module


def import_mantid(modulename, package=""):
    """
    Import a Mantid module built from the PythonInterface.

    This checks for the module in either current working directory (for development builds)
    or `current_working_directory/mantid/...` for packaged builds.
    :param modulename:
    :param package:
    :param attr:
    :return: The module if successfully imported
    :raises: If the module cannot be imported
    """

    if modulename.startswith('.'):
        try:
            lib = import_module(modulename, package)
        except ImportError as e1:
            try:
                lib = import_module(modulename.lstrip('.'))
            except ImportError as e2:
                msg = 'import of "{}" failed with "{}"'
                msg = 'First ' + msg.format(modulename, e1) \
                      + '. Second ' + msg.format(modulename.lstrip('.'), e2)
                raise ImportError(msg)
    else:
        lib = import_module(modulename)

    return lib
