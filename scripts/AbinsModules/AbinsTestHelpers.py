from __future__ import (absolute_import, division, print_function)
import re
import platform
import os
import AbinsModules


# Module with helper functions used to create tests.
def version_as_tuple(string=None):
    """
    Calculates numerical representation of package version as a tuple.

    :param string:  version of package in the format number1.number2.number3....numberN
    :return: numerical representation of package version in the form  of tuple (number1, number2 ...., numberN)
    """
    if not isinstance(string, str):
        raise ValueError("Version of package in the form of string is expected.")
    if "." not in string:
        raise ValueError("Invalid format of package version.")

    try:
        return tuple([int(i) for i in re.findall(r'\d+', string=string)])
    except:
        raise ValueError("Version of package couldn't be converted to number. (version=", string)


def is_numpy_valid(string=None):
    """

    :param string: version of numpy to be checked in the format number1.number2.number3
    :return: False if version of numpy  is valid otherwise True
    """

    return version_as_tuple(string=string) < version_as_tuple(AbinsModules.AbinsConstants.NUMPY_VERSION_REQUIRED)


def old_python():
    """
    Checks if Python i not too old
    :return: True if it is too old otherwise False
    """
    return tuple([int(i) for i in re.findall(r'\d+', string=platform.python_version().replace(".", " "))]) < (2, 7)


def find_file(filename=None):
    """
    Calculates path of filename with the testing data. Path is determined in the platform independent way.

    :param filename: name of file to find
    :return: full path for the file with the testing data
    """
    from mantid.api import FileFinder
    return FileFinder.Instance().getFullPath(filename)


def remove_output_files(list_of_names=None):
    """Removes output files created during a test."""
    if not isinstance(list_of_names, list):
        raise ValueError("List of names is expected.")
    if not all(isinstance(i, str) for i in list_of_names):
        raise ValueError("Each name should be a string.")

    files = os.listdir(os.getcwd())
    for filename in files:
        for name in list_of_names:
            if name in filename:
                os.remove(filename)
                break
