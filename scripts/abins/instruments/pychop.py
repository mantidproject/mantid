from __future__ import (absolute_import, division, print_function)
import math
from typing import Optional

import numpy as np
from mantidqtinterfaces.PyChop import Instruments as pychop_instruments

import abins
from abins.constants import FLOAT_TYPE, MILLI_EV_TO_WAVENUMBER, WAVENUMBER_TO_INVERSE_A
from abins.sdata import SDataByAngle
from .instrument import Instrument
from .broadening import broaden_spectrum, prebin_required_schemes


class PyChopInstrument(Instrument):
    """Simulated direct-geometry INS with PyChop2

    PyChop is used to compute energy resolution as a function of energy for
    given instrument settings.

    The "tthlims" data from PyChop is used to determine sampling angles.
    """
    def __init__(self, name='MAPS', setting='', chopper_frequency=400):
        self._name = name
        self._e_init = None
        self._chopper = setting
        self._chopper_frequency = chopper_frequency
        self._polyfits = {}
        self._tthlims = pychop_instruments.Instrument(self._name).detector.tthlims

        super().__init__(setting=setting)

    def get_angles(self):
        parameters = abins.parameters.instruments[self._name]

        angle_ranges = [(start, end) for (start, end) in zip(self._tthlims[0::2],
                                                             self._tthlims[1::2])]
        angles = np.concatenate([np.linspace(start, end, parameters['angles_per_detector'])
                                 for (start, end) in angle_ranges])
        return angles

    def calculate_q_powder(self, *, input_data=None, angle=None):
        """
        Returns momentum transfer Q^2 corresponding to frequency series

        using cosine rule Q^2 = a^2 + b^2 - 2 ab cos(theta)
        where a, b are k_i and k_f

        Calculation is restricted to the region E < E_i

        :param input_data: list with frequencies for the given k-point.
        :param angle: scattering angle in degrees
        :type array-like:
        """

        # Set momentum transfer to zero for energy transitions larger than the
        # incident energy of neutrons and calculate momentum transfer according
        # to momentum conservation principle for energy transitions smaller than
        # the incident energy of neutrons.

        k2_f = np.zeros_like(input_data)
        k2_i = np.zeros_like(input_data)

        conservation_indx = self._e_init > input_data

        k2_f[conservation_indx] = (self._e_init - input_data[conservation_indx]) * WAVENUMBER_TO_INVERSE_A
        k2_i[conservation_indx] = self._e_init * WAVENUMBER_TO_INVERSE_A

        cos_angle = math.cos(math.radians(angle))
        result = k2_i + k2_f - 2 * cos_angle * (k2_i * k2_f) ** 0.5

        return result

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, scheme='auto'):
        """
        Convolve discrete frequency spectrum with the resolution function for the TwoDMap instrument.

        - The broadening parameters are set in abins.parameters.instruments['TwoDMap']

        :param frequencies: DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT
        """

        if scheme == 'auto':
            scheme = 'interpolate'

        if scheme in prebin_required_schemes:
            s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
            frequencies = (bins[1:] + bins[:-1]) / 2

        sigma = np.full(frequencies.size, self._calculate_sigma(frequencies),
                        dtype=FLOAT_TYPE)

        points_freq, broadened_spectrum = broaden_spectrum(frequencies, bins, s_dft, sigma,
                                                           scheme=scheme)
        return points_freq, broadened_spectrum

    def set_incident_energy(self, e_init=None):
        """
        Setter for incident energy.
        :param e_init: new incident energy
        """
        if isinstance(e_init, float):
            self._e_init = e_init
        else:
            raise ValueError("Invalid value of incident energy (%s, type(%s) = %s; should be float)."
                             % (e_init, e_init, type(e_init)))

    def _calculate_sigma(self, frequencies):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        if self._e_init not in self._polyfits:
            self._polyfit_resolution()

        return np.polyval(self._polyfits[self._e_init], frequencies)

    def _polyfit_resolution(self, n_values=40, order=4):
        from PyChop import PyChop2
        from abins.constants import MILLI_EV_TO_WAVENUMBER

        frequencies_invcm = np.linspace(0, self._e_init, n_values, endpoint=False)
        frequencies_mev = frequencies_invcm / MILLI_EV_TO_WAVENUMBER
        ei_mev = self._e_init / MILLI_EV_TO_WAVENUMBER

        setting_params = abins.parameters.instruments[self._name]['settings'][self._setting]

        resolution, _ = PyChop2.calculate(inst=self._name,
                                          package=setting_params['chopper'],
                                          freq=self._chopper_frequency,
                                          ei=ei_mev,
                                          etrans=frequencies_mev.tolist())
        fit = np.polyfit(frequencies_invcm, resolution / MILLI_EV_TO_WAVENUMBER, order)

        self._polyfits[self._e_init] = fit

    def save_nxspe(self, data: SDataByAngle,
                   filename: str = 'abins.nxspe'):

        import h5py
        import time

        distances = {'MARI': 4.021999835968018}
        if self.get_name() not in distances:
            raise ValueError(f"Cannot write nxspe for instrument {self.get_name()}, "
                             "detector distance(s) unknown.")

        # Get regular bins from frequencies; for some reason SData doesn't store them
        bin_width = data[0].get_bin_width()
        bins = np.concatenate([data.frequencies[:1] - bin_width / 2,
                               data.frequencies + bin_width / 2])

        n_detectors = len(self._tthlims) // 2
        angles_per_detector = abins.parameters.instruments[self._name]['angles_per_detector']
        angle_ranges = [(start, end) for (start, end) in zip(self._tthlims[0::2],
                                                             self._tthlims[1::2])]
        detector_midpoint_angles = [(end + start) / 2 for (start, end) in angle_ranges]
        detector_widths = [(end - start) for (start, end) in angle_ranges]

        summed_data = np.zeros((n_detectors, len(data.frequencies)))
        for detector_index in range(n_detectors):
            for sdata in data[(detector_index * angles_per_detector)
                              :((detector_index + 1) * angles_per_detector)]:
                summed_data[detector_index, :] += sdata.get_total_intensity()

        def _create_entry(root, entry_type: str,
                          entry_name: str,
                          attrs: Optional[dict] = None,
                          **kwargs) -> h5py.Group:
            entry_functions = {'dataset': 'create_dataset',
                               'group': 'create_group'}

            entry = getattr(root, entry_functions[entry_type])(entry_name, **kwargs)

            if attrs is not None:
                for key, value in attrs.items():
                    if isinstance(value, str):
                        value = np.array(value, dtype='S')  # Cast strings to fixed-length
                    entry.attrs[key] = value
            return entry

        with h5py.File(filename, 'w') as f:
            for key, value in {'HDF5_Version': '1.8.15',
                               'NeXus_version': '4.3.2',
                               'file_name': filename,
                               'file_time': time.strftime('%Y-%m-%dT%H:%M:%S+00:00', time.gmtime())
                               }.items():
                f.attrs[key] = np.array(value, dtype='S')

            root = _create_entry(f, 'group', 'SyntheticData',
                                 attrs={'NX_class': 'NXentry'})

            _create_entry(root, 'dataset', 'definition',
                          data=np.array('NXSPE', dtype='S'), shape=(1,), attrs={'version': '1.2'})
            _create_entry(root, 'dataset', 'program_name',
                          data=np.array('mantid', dtype='S'), shape=(1,), attrs={'version': '3.13.0'})

            _create_entry(root, 'group', 'sample', attrs={'NX_class': 'NXsample'})

            nxspe_info = _create_entry(root, 'group', 'NXSPE_info', attrs={'NX_class': 'NXcollection'})
            _create_entry(nxspe_info, 'dataset', 'fixed_energy',
                          data=(np.array([self._e_init]) / MILLI_EV_TO_WAVENUMBER), dtype=np.float64,
                          attrs={'units': 'meV'})
            _create_entry(nxspe_info, 'dataset', 'ki_over_kf_scaling', shape=(1,),
                          data=1, dtype=np.int32)
            _create_entry(nxspe_info, 'dataset', 'psi', data=np.array(['NaN'], dtype=np.float64),
                          attrs={'units': 'degrees'})

            data_group = _create_entry(root, 'group', 'data', attrs={'NX_class': 'NXdata'})

            # Azimuthal values are not used in this model, fill with dummy values
            _create_entry(data_group, 'dataset', 'azimuthal', data=np.zeros(n_detectors))
            _create_entry(data_group, 'dataset', 'azimuthal_width', data=np.full(n_detectors, 45.))

            _create_entry(data_group, 'dataset', 'data',
                          data=summed_data, attrs={'axes': 'polar:energy', 'signal': 1})
            _create_entry(data_group, 'dataset', 'distance',
                          data=np.full(n_detectors, distances[self.get_name()]))
            _create_entry(data_group, 'dataset', 'energy',
                          data=(bins / MILLI_EV_TO_WAVENUMBER), attrs={'units': 'meV'})
            _create_entry(data_group, 'dataset', 'error', data=np.zeros_like(summed_data))
            _create_entry(data_group, 'dataset', 'polar',
                          data=np.array(detector_midpoint_angles, dtype=np.float64))
            _create_entry(data_group, 'dataset', 'polar_width',
                          data=np.array(detector_widths), dtype=np.float64)

            instrument = _create_entry(root, 'group', 'instrument', attrs={'NX_class': 'NXinstrument'})
            _create_entry(instrument, 'dataset', 'name',
                          data=np.array(self.get_name(), dtype='S'), shape=(1,),
                          attrs={'short_name': self.get_name()})

            instrument_fermi = _create_entry(instrument, 'group', 'fermi', attrs={'NX_class': 'NXfermi_chopper'})
            _create_entry(instrument_fermi, 'dataset', 'energy', data=self._e_init, dtype=np.float64)
