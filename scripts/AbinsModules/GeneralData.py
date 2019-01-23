# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class GeneralData(object):

    def __init__(self):
        self._data = []  # data

    # noinspection PyUnusedLocal
    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """

        return None

    def extract(self):
        """
        Returns the data.
        :returns: data
        """
        return None

    def __str__(self):
        """
        String representation of class instances. Has to be implemented by inheriting classes.
        """
        return None
