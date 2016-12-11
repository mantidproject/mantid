import re
from platform import python_version
from mantid import config
import AbinsConstants


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

    return version_as_tuple(string=string) < version_as_tuple(AbinsConstants.NUMPY_VERSION_REQUIRED)


def old_python():
    """
    Checks if Python i not too old
    :return: True if it is too old otherwise False
    """
    return tuple([int(i) for i in re.findall(r'\d+', string=python_version().replace(".", " "))]) < (2, 7)


def get_core_folder():
    """
    Calculates folder in which testing data is stored. Folder is determined in the platform independent way. It is
    assumed that the path to the testing data includes keyword  "UnitTest".
    :return: path to the folder with testing data
    """
    folders = config.getDataSearchDirs()
    for folder in folders:
        if "UnitTest" in folder:
            return folder
    raise ValueError("Folder with testing data not found.")
