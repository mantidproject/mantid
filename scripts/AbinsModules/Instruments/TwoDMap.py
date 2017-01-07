import numpy as np
import math

from Instrument import Instrument
from AbinsModules import AbinsParameters, AbinsConstants
from AbinsModules.KpointsData import KpointsData


class TwoDMap(Instrument):
    def __init__(self, name):

        super(TwoDMap, self).__init__()
        self._name = name
        self._sigma = AbinsParameters.delta_width
        q_points = self._make_q_point_mesh()
        temp_q_powder = np.ravel(np.multiply(q_points, q_points))
        self._q_powder, self._q_powder_multipliticies = self._calculate_multiplicities(q_powder=temp_q_powder)

    def calculate_q_powder(self, input_data=None):
        """
        Returns Q powder data for index input_data.
        :param input_data: index of Q2
        """
        if isinstance(input_data, (int, long)) and 0 <= input_data < self._q_powder.size:

            return self._q_powder[input_data]
        else:
            raise ValueError("Invalid Q index.")

    def _calculate_multiplicities(self, q_powder=None):

        ones = np.ones(shape=q_powder, dtype=AbinsConstants.FLOAT_TYPE)
        bins = np.arange(start=AbinsParameters.q2_start, step=AbinsParameters.q2_step, stop=AbinsParameters.q2_end,
                         dtype=AbinsConstants.FLOAT_TYPE)
        indices = np.digitize(q_powdern, bins=bins)
        multiplicities =np.asarray(a=[ones[indices == i].sum() for i in range(AbinsConstants.FIRST_BIN_INDEX, bins)],
                                   dtype=AbinsConstants.FLOAT_TYPE)

        return bins[AbinsConstants.FIRST_BIN_INDEX:], multiplicities

    def get_q_powder_size(self):
        return self._q_powder.size

    def _make_q_point_mesh(self):
        """
        Constructs Q vectors from Q grid.
        @return:
        """
        if AbinsParameters.q_mesh[0] % 2:
            i1min = - (AbinsParameters.q_mesh[0] - 1) / 2
            i1max = (AbinsParameters.q_mesh[0] - 1) / 2
        else:
            i1min = -(AbinsParameters.q_mesh[0] / 2 - 1)
            i1max = AbinsParameters.q_mesh[0] / 2

        if AbinsParameters.q_mesh[1] % 2:
            i2min = - (AbinsParameters.q_mesh[1] - 1) / 2
            i2max = (AbinsParameters.q_mesh[1] - 1) / 2
        else:
            i2min = -(AbinsParameters.q_mesh[1] / 2 - 1)
            i2max = AbinsParameters.q_mesh[1] / 2

        if AbinsParameters.q_mesh[2] % 2:
            i3min = - (AbinsParameters.q_mesh[2] - 1) / 2
            i3max = (AbinsParameters.q_mesh[2] - 1) / 2

        else:
            i3min = -(AbinsParameters.q_mesh[2] / 2 - 1)
            i3max = AbinsParameters.q_mesh[2] / 2

        dimensions = 3
        total_q_num = AbinsParameters.q_mesh[0] * AbinsParameters.q_mesh[1] * AbinsParameters.q_mesh[2]
        q_point_mesh = np.zeros((total_q_num, dimensions), dtype=AbinsParameters.float_type)
        n_q = 0

        for i1 in range(i1min, i1max + 1):
            for i2 in range(i2min, i2max + 1):
                for i3 in range(i3min, i3max + 1):
                    q_point_mesh[n_q, :] = [float(i1) / float(AbinsParameters.q_mesh[0]),
                                            float(i2) / float(AbinsParameters.q_mesh[1]),
                                            float(i3) / float(AbinsParameters.q_mesh[2])]
                n_q += 1

        _num_k = self._k_points_data.extract()["frequencies"].shape[0]
        q_points = np.zeros((_num_k, total_q_num, dimensions), dtype=AbinsParameters.float_type)

        for _k in range(_num_k):
            q_points[_k, :, :] = q_point_mesh  # no need to use np.copy here

        return q_points

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None, start=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak of broadened spectrum
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """

        if AbinsParameters.pkt_per_peak == 1:

            points_freq = frequencies
            broadened_spectrum = s_dft

        else:

            # freq_num: number of transition energies for the given quantum order event

            # frequencies_matrix[freq_num, AbinsParameters.pkt_per_peak]
            frequencies_matrix = np.array([frequencies, ] * AbinsParameters.pkt_per_peak).transpose()

            # delta_val[freq_num, AbinsParameters.pkt_per_peak]
            delta_val = self._dirac_delta(center=frequencies_matrix)

            # s_dft_matrix[freq_num, AbinsParameters.pkt_per_peak]
            s_dft_matrix = np.array([s_dft, ] * AbinsParameters.pkt_per_peak).transpose()

            # temp_spectrum[freq_num, AbinsParameters.pkt_per_peak]
            temp_spectrum = s_dft_matrix * delta_val

            points_freq = np.ravel(broad_freq)
            broadened_spectrum = np.ravel(temp_spectrum)

        return points_freq, broadened_spectrum

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
