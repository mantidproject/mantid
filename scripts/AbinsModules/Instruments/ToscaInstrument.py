# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import numpy as np
from .Instrument import Instrument
from .Broadening import broaden_spectrum
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants
from AbinsModules.FrequencyPowderGenerator import FrequencyPowderGenerator


class ToscaInstrument(Instrument, FrequencyPowderGenerator):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    parameters = AbinsParameters.instruments['TOSCA']

    def __init__(self, name):
        self._name = name
        super(ToscaInstrument, self).__init__()

    @classmethod
    def calculate_q_powder(cls, input_data=None):
        """Calculates squared Q vectors for TOSCA and TOSCA-like instruments.

        By the cosine law Q^2 = k_f^2 + k_i^2 - 2 k_f k_i cos(theta)

        where k are determined from
        AbinsParameters.instruments['TOSCA']['final_neutron_energy']
        and the input series of vibrational frequencies and cos(theta) is
        precomputed as AbinsParameters.instruments['TOSCA']['cos_scattering_angle']

        :param input_data:
            frequencies (in cm-1) which should be used to construct Q2

        :returns:
            Q^2 array (in cm-1) corresponding to input frequencies,
            constrained by conservation of mass/momentum and TOSCA geometry
        """

        k2_i = (input_data + cls.parameters['final_neutron_energy']) * AbinsConstants.WAVENUMBER_TO_INVERSE_A
        k2_f = cls.parameters['final_neutron_energy'] * AbinsConstants.WAVENUMBER_TO_INVERSE_A
        result = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * cls.parameters['cos_scattering_angle']
        return result

    @classmethod
    def get_sigma(cls, frequencies):
        """Frequency-dependent broadening width from empirical fit"""
        a = cls.parameters['a']
        b = cls.parameters['b']
        c = cls.parameters['c']
        sigma = a * frequencies ** 2 + b * frequencies + c
        return sigma

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, prebin='auto'):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        :param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm^-1)
        :param bins: Evenly-spaced frequency bin values for the output spectrum.
        :type bins: 1D array-like
        :param s_dft:  discrete S calculated directly from DFT
        :param prebin:
            Bin the data before convolution. This greatly reduces the workload for large sets of frequencies, but loses
            a little precision. If set to 'auto', a choice is made based on the relative numbers of frequencies and
            sampling bins. For 'legacy' broadening this step is desregarded and implemented elsewhere.
        :type prebin: str or bool

        :returns: (points_freq, broadened_spectrum)
        """

        prebin_required_schemes = ['interpolate', 'interpolate_coarse', 'conv_15']

        if AbinsParameters.sampling['pkt_per_peak'] == 1:

            points_freq = frequencies
            broadened_spectrum = s_dft

        else:

            scheme = AbinsParameters.sampling['broadening_scheme']

            if prebin == 'auto':
                if bins.size < frequencies.size:
                    prebin = True
                elif scheme in prebin_required_schemes:
                    prebin = True
                else:
                    prebin = False
            if prebin is True:
                s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
                frequencies = (bins[1:] + bins[:-1]) / 2
            elif prebin is False:
                if scheme in prebin_required_schemes:
                    raise ValueError('"prebin" should not be set to False when using "{}" broadening scheme'.format(scheme))
            else:
                raise ValueError('"prebin" option must be True, False or "auto"')

            sigma = self.get_sigma(frequencies)

            points_freq, broadened_spectrum = broaden_spectrum(frequencies=frequencies,
                                                               s_dft=s_dft,
                                                               bins=bins,
                                                               sigma=sigma,
                                                               scheme=scheme)
        return points_freq, broadened_spectrum
