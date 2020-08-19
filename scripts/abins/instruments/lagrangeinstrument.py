# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import abins
from abins.constants import MILLI_EV_TO_WAVENUMBER, WAVENUMBER_TO_INVERSE_A
from .indirectinstrument import IndirectInstrument

class LagrangeInstrument(IndirectInstrument):
    """Instrument class for IN1-LAGRANGE instrument at ILL

    """
    parameters = abins.parameters.instruments['Lagrange']

    def __init__(self, name, setting='Cu(220)'):
        super().__init__(name=name, setting=setting)

        if setting in self.parameters['settings']:
            self._setting = setting
        else:
            raise ValueError('Setting "{}" is uknown for Lagrange '
                             'instrument. Supported settings: {}'.format(
                                 setting,
                                 ', '.join(sorted(self.parameters['settings'].keys()))))

    def get_sigma(self, frequencies):
        ei_resolution = self.parameters['settings'][self._setting].get('ei_resolution', 0)
        abs_resolution_meV = self.parameters['settings'][self._setting].get('abs_resolution_meV', 0)
        abs_resolution_cm = abs_resolution_meV * MILLI_EV_TO_WAVENUMBER

        return frequencies * ei_resolution + abs_resolution_cm

    @classmethod
    def calculate_q_powder(cls, input_data=None):
        """Calculates squared Q vectors for Lagrange.

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

        print("WARNING: WORK IN PROGRESS. THIS Q POWDER MODEL IS FOR TOSCA NOT LAGRANGE")
        k2_i = (input_data + cls.parameters['final_neutron_energy']) * WAVENUMBER_TO_INVERSE_A
        k2_f = cls.parameters['final_neutron_energy'] * WAVENUMBER_TO_INVERSE_A
        result = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * cls.parameters['cos_scattering_angle']
        return result
