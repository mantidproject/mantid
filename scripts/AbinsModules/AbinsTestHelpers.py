# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os


# Module with helper functions used to create tests.
def find_file(filename=None):
    """
    Calculates path of filename with the testing data. Path is determined in the platform independent way.

    :param filename: name of file to find
    :returns: full path for the file with the testing data
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
