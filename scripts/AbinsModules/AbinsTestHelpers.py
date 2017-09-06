from __future__ import (absolute_import, division, print_function)
import re
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

    # import ConfigService here to avoid:
    # RuntimeError: Pickling of "mantid.kernel._kernel.ConfigServiceImpl"
    # instances is not enabled (http://www.boost.org/libs/python/doc/v2/pickle.html)

    from mantid.kernel import ConfigService

    if not isinstance(list_of_names, list):
        raise ValueError("List of names is expected.")
    if not all(isinstance(i, str) for i in list_of_names):
        raise ValueError("Each name should be a string.")

    save_dir_path = ConfigService.getString("defaultsave.directory")
    if save_dir_path != "":  # default save directory set
        all_files = os.listdir(save_dir_path)
    else:
        all_files = os.listdir(os.getcwd())

    for filename in all_files:
        for name in list_of_names:
            if name in filename:
                full_path = os.path.join(save_dir_path, filename)
                if os.path.isfile(full_path):
                    os.remove(full_path)
                break
