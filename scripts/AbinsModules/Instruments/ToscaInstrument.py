from __future__ import (absolute_import, division, print_function)

from .Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants
from AbinsModules.FrequencyPowderGenerator import FrequencyPowderGenerator


class ToscaInstrument(Instrument, FrequencyPowderGenerator):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name
        super(ToscaInstrument, self).__init__()

    def calculate_q_powder(self, input_data=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        :param input_data: frequencies which should be used to construct Q2
        """

        k2_i = (input_data + AbinsParameters.tosca_final_neutron_energy) * AbinsConstants.WAVENUMBER_TO_INVERSE_A
        k2_f = AbinsParameters.tosca_final_neutron_energy * AbinsConstants.WAVENUMBER_TO_INVERSE_A
        result = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * AbinsParameters.tosca_cos_scattering_angle
        return result

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        :param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm^-1)
        :param s_dft:  discrete S calculated directly from DFT
        """
        if AbinsParameters.pkt_per_peak == 1:

            points_freq = frequencies
            broadened_spectrum = s_dft

        else:

            # freq_num: number of transition energies for the given quantum order event
            # sigma[freq_num]

            a = AbinsParameters.tosca_a
            b = AbinsParameters.tosca_b
            c = AbinsParameters.tosca_c
            sigma = a * frequencies ** 2 + b * frequencies + c

            pkt_per_peak = AbinsParameters.pkt_per_peak
            points_freq, broadened_spectrum = self._convolve_with_resolution_function_helper(frequencies=frequencies,
                                                                                             s_dft=s_dft,
                                                                                             sigma=sigma,
                                                                                             pkt_per_peak=pkt_per_peak,
                                                                                             gaussian=self._gaussian)
        return points_freq, broadened_spectrum
