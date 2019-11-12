# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import AbinsModules
import numpy as np


# noinspection PyPep8Naming
class Instrument(object):

    _name = None

    def calculate_q_powder(self, input_data=None):
        """
        Calculates Q2.


        :param  input_data: data from which Q2 should be calculated
        :returns:  numpy array with Q2 data
        """

        return None

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        :param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT

       """
        return None


    def __str__(self):
        return self._name

    def get_name(self):
        return self._name
