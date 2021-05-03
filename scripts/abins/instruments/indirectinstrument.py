# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

import abins
from abins.constants import WAVENUMBER_TO_INVERSE_A
from .instrument import Instrument
from .broadening import broaden_spectrum, prebin_required_schemes


class IndirectInstrument(Instrument, abins.FrequencyPowderGenerator):
    """
    Class for instruments with energy-dependent resolution function
    """
    def __init__(self, name: str, setting: str = ''):
        self._name = name
        super().__init__(setting=setting)

    def get_angles(self):
        parameters = abins.parameters.instruments[self._name]
        if ('settings' in parameters
                and 'angles' in parameters['settings'][self.get_setting()]):
            return parameters['settings'][self.get_setting()]['angles']
        else:
            return parameters['angles']

    def calculate_q_powder(self, *, input_data=None, angle=None):
        """Calculates squared Q vectors.

        By the cosine law Q^2 = k_f^2 + k_i^2 - 2 k_f k_i cos(theta)

        where k are determined from
        abins.parameters.instruments['TOSCA']['final_neutron_energy']
        and the input series of vibrational frequencies and cos(theta) is
        precomputed as abins.parameters.instruments['TOSCA']['cos_scattering_angle']

        :param input_data:
            frequencies (in cm-1) which should be used to construct Q2
        :param angle:
            detector angle in degrees

        :returns:
            Q^2 array (in cm-1) corresponding to input frequencies,
            constrained by conservation of mass/momentum and TOSCA geometry
        """
        parameters = abins.parameters.instruments[self._name]

        cos_scattering_angle = np.cos(angle * np.pi / 180)

        k2_i = (input_data + parameters['final_neutron_energy']) * WAVENUMBER_TO_INVERSE_A
        k2_f = parameters['final_neutron_energy'] * WAVENUMBER_TO_INVERSE_A
        result = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * cos_scattering_angle
        return result

    @classmethod
    def get_sigma(cls, frequencies):
        raise NotImplementedError()

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, scheme='auto', prebin='auto'):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        :param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm^-1)
        :param bins: Evenly-spaced frequency bin values for the output spectrum.
        :type bins: 1D array-like
        :param s_dft:  discrete S calculated directly from DFT
        :param scheme: Broadening scheme. This is passed to ``Instruments.Broadening.broaden_spectrum()`` unless set to
            'auto'. If set to 'auto', the scheme will be based on the number of frequency values: 'gaussian_windowed' is
            used for small numbers of peaks, while richer spectra will use the fast approximate 'interpolate' scheme.
            To avoid any approximation or truncation, the 'gaussian' and 'normal' schemes are the most accurate, but
            will run considerably more slowly.
        :param prebin:
            Bin the data before convolution. This greatly reduces the workload for large sets of frequencies, but loses
            a little precision. If set to 'auto', a choice is made based on the relative numbers of frequencies and
            sampling bins. For 'legacy' broadening this step is desregarded and implemented elsewhere.
        :type prebin: str or bool

        :returns: (points_freq, broadened_spectrum)
        """

        if scheme == 'auto':
            if frequencies.size > 50:
                selected_scheme = 'interpolate'
            else:
                selected_scheme = 'gaussian_truncated'
        else:
            selected_scheme = scheme

        if prebin == 'auto':
            if bins.size < frequencies.size:
                prebin = True
            elif selected_scheme in prebin_required_schemes:
                prebin = True
            else:
                prebin = False

        if prebin is True:
            s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
            frequencies = (bins[1:] + bins[:-1]) / 2
        elif prebin is False:
            if selected_scheme in prebin_required_schemes:
                raise ValueError('"prebin" should not be set to False when using "{}" broadening scheme'.format(scheme))
        else:
            raise ValueError('"prebin" option must be True, False or "auto"')

        sigma = self.get_sigma(frequencies)

        points_freq, broadened_spectrum = broaden_spectrum(frequencies, bins, s_dft,
                                                           sigma, scheme=selected_scheme)
        return points_freq, broadened_spectrum
