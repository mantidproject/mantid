import numpy as np
import math

from Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules.KpointsData import KpointsData

class NoneInstrument(Instrument):
    def __init__(self, name):
        self._name = name
        self._k_points_data = None
        self._sigma = AbinsParameters.delta_width
        super(NoneInstrument, self).__init__()

    def collect_K_data(self, k_points_data=None):
        """
        Collect k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")

        self._k_points_data = k_points_data


    def calculate_q_powder(self, quantum_order_events_num=None):
        """
        Calculates squared Q vectors for None instrument.
        """
        q_points = self._make_q_point_mesh()
        return np.multiply(q_points, q_points)


    def _make_q_point_mesh(self):
        """
        Constructs Q vectors from Q grid.
        @return:
        """

        if  AbinsParameters.q_mesh[0] % 2 :
            i1min = - (AbinsParameters.q_mesh[0] - 1) / 2
            i1max = (AbinsParameters.q_mesh[0] - 1) / 2
        else:
            i1min = -(AbinsParameters.q_mesh[0] / 2 - 1)
        i1max = AbinsParameters.q_mesh[0] / 2


        if  AbinsParameters.q_mesh[1] % 2 :
            i2min = - (AbinsParameters.q_mesh[1] - 1) / 2
            i2max = (AbinsParameters.q_mesh[1] - 1) / 2
        else:
            i2min = -(AbinsParameters.q_mesh[1] / 2 - 1)
            i2max = AbinsParameters.q_mesh[1] / 2


        if  AbinsParameters.q_mesh[2] % 2 :
            i3min = - (AbinsParameters.q_mesh[2] - 1) / 2
            i3max = (AbinsParameters.q_mesh[2] - 1) / 2

        else:
            i3min = -(AbinsParameters.q_mesh[2] / 2 - 1)
            i3max = AbinsParameters.q_mesh[2] / 2

        _dimensions = 3
        _total_q_num = AbinsParameters.q_mesh[0] * AbinsParameters.q_mesh[1] * AbinsParameters.q_mesh[2]
        _q_point_mesh = np.zeros((_total_q_num, _dimensions), dtype=AbinsParameters.float_type)
        _n_q = 0

        for _i1 in range(i1min, i1max + 1):
            for _i2 in range(i2min, i2max + 1):
                for _i3 in range(i3min, i3max + 1):
                    _q_point_mesh[_n_q, :] =[float(_i1) / float(AbinsParameters.q_mesh[0]),
                                             float(_i2) / float(AbinsParameters.q_mesh[1]),
                                             float(_i3) / float(AbinsParameters.q_mesh[2])]
                _n_q += 1

        _num_k = self._k_points_data.extract()["frequencies"].shape[0]
        q_points = np.zeros((_num_k, _total_q_num, _dimensions ), dtype=AbinsParameters.float_type)

        for _k in range(_num_k):
            q_points[_k,:,:] = _q_point_mesh # no need to use np.copy here

        return q_points


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None, start=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak of broadened spectrum
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """

        # noinspection PyTypeChecker
        all_points = points_per_peak * frequencies.shape[0]

        broadened_spectrum = np.zeros(all_points, dtype=AbinsParameters.float_type)
        start_broaden = start * points_per_peak
        for indx, freq in np.ndenumerate(frequencies[start:]):

            start = indx[0] * points_per_peak + start_broaden
            broadened_spectrum[start:start + points_per_peak] = np.convolve(s_dft[indx[0]], self._dirac_delta(center=freq))

        return broadened_spectrum


    def produce_abscissa(self, frequencies=None, points_per_peak=None, start=None):
        """
        Creates abscissa for convoluted spectrum in case of TOSCA.
        @param frequencies:  DFT frequencies for which frequencies which correspond to broadened spectrum should be regenerated (frequencies in cm-1)
        @param points_per_peak:  number of points for each peak of broadened spectrum
        @return: abscissa for convoluted S
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """
        all_points = points_per_peak * frequencies.shape[0]
        abscissa = np.zeros(all_points, dtype=AbinsParameters.float_type)
        # noinspection PyTypeChecker
        start_broaden = start * points_per_peak

        for indx, freq in np.ndenumerate(frequencies[start:]):

            points_freq = np.array(np.linspace(freq - AbinsParameters.fwhm * self._sigma,
                                               freq + AbinsParameters.fwhm * self._sigma,
                                               num=points_per_peak))

            start = indx[0] * points_per_peak + start_broaden
            abscissa[start:start + points_per_peak] = points_freq

        return abscissa


    def _dirac_delta(self, center=None):

        """
        Dirac delta is implemented as a very narrow Gaussian function.
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """

        points = np.array(np.linspace(center - AbinsParameters.fwhm * self._sigma,
                                      center + AbinsParameters.fwhm * self._sigma,
                                      num=AbinsParameters.points_per_peak))

        sigma_factor = 2.0 * self._sigma * self._sigma

        # noinspection PyTypeChecker
        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-np.power(points - center, 2) / sigma_factor)
