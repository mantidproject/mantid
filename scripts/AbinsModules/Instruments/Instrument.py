# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


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

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, scheme='auto'):
        """
        Convolves discrete spectrum with the resolution function for the particular instrument.

        :param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        :type frequencies: 1D array-like
        :param bins: Bin edges for output histogram. Most broadening implementations expect this to be regularly-spaced.
        :type bins: 1D array-like
        :param s_dft: discrete S calculated directly from DFT
        :type s_dft: 1D array-like
        :param scheme: Broadening scheme. Multiple implementations are available in *Instruments.Broadening* that trade-
            off between speed and accuracy. Not all schemes must (or should?) be implemented for all instruments, but
            'auto' should select something sensible.
        :type scheme: str

       """
        return None

    def __str__(self):
        return self._name

    def get_name(self):
        return self._name
