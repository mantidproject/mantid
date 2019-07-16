# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from __future__ import absolute_import

from mantid.py3compat import Enum


# Any changes here must be reflected in the definition in
# the C++ MplCpp/Plot.h header. See the comment in that file
# for the reason for duplication.
class MantidAxType(Enum):
    """Define an list of possible axis types for plotting"""

    BIN = 0
    SPECTRUM = 1


def find_errorbar_container(line, containers):
    """
    Finds the ErrorbarContainer associated with the plot line.

    :param line: Line that is looked for
    :param containers: Collection of containers that contain `ErrorbarContainer`s
    :return: The container that contains the line
    """
    for container in containers:
        if line == container[0]:
            return container
