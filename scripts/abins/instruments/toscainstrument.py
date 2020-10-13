# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

import abins
import abins.parameters
from abins.constants import WAVENUMBER_TO_INVERSE_A
from .indirectinstrument import IndirectInstrument
from .broadening import broaden_spectrum, prebin_required_schemes


class ToscaInstrument(IndirectInstrument):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    parameters = abins.parameters.instruments['TOSCA']

    def __init__(self, setting: str = ''):
        super().__init__(name='TOSCA', setting=setting)

        if self._setting != '':
            raise ValueError('TOSCA Instrument does not use multiple settings')

    @classmethod
    def calculate_q_powder(cls, input_data=None):
        """Calculates squared Q vectors for TOSCA and TOSCA-like instruments.

        By the cosine law Q^2 = k_f^2 + k_i^2 - 2 k_f k_i cos(theta)

        where k are determined from
        abins.parameters.instruments['TOSCA']['final_neutron_energy']
        and the input series of vibrational frequencies and cos(theta) is
        precomputed as abins.parameters.instruments['TOSCA']['cos_scattering_angle']

        :param input_data:
            frequencies (in cm-1) which should be used to construct Q2

        :returns:
            Q^2 array (in cm-1) corresponding to input frequencies,
            constrained by conservation of mass/momentum and TOSCA geometry
        """

        k2_i = (input_data + cls.parameters['final_neutron_energy']) * WAVENUMBER_TO_INVERSE_A
        k2_f = cls.parameters['final_neutron_energy'] * WAVENUMBER_TO_INVERSE_A
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
