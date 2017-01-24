import numpy as np

from Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants


class TwoDMap(Instrument):
    def __init__(self, name):

        super(TwoDMap, self).__init__()
        self._name = name
        self._sigma = AbinsParameters.delta_width
        q_points = self._make_q_point_mesh()
        temp_q_powder = np.ravel(q_points * q_points)
        self._calculate_multiplicities(q_powder=temp_q_powder)

    def calculate_q_powder(self, input_data=None):
        """
        Returns Q powder data for index input_data.
        :param input_data: index of Q2
        """
        if isinstance(input_data, (int, long)) and 0 <= input_data < self._q_powder.size:

            return self._q_powder[input_data]
        else:
            raise ValueError("Invalid Q index. (q_index = %s )" % input_data)

    # noinspection PyMethodMayBeStatic
    def _calculate_multiplicities(self, q_powder=None):
        """
        Calculates how many times each element of Q2 occurs.
        :param q_powder: 1D array with Q2
        """

        ones = np.ones(shape=q_powder.size, dtype=AbinsConstants.FLOAT_TYPE)
        bins = np.arange(start=AbinsParameters.q2_start, step=AbinsParameters.q2_step, stop=AbinsParameters.q2_end,
                         dtype=AbinsConstants.FLOAT_TYPE)

        if q_powder.size > bins.size:

            indices = np.digitize(q_powder, bins=bins)
            multiplicities = np.asarray(a=[ones[indices == i].sum()
                                           for i in range(AbinsConstants.FIRST_BIN_INDEX, bins.size)],
                                        dtype=AbinsConstants.FLOAT_TYPE)
            q2_rebined = bins[AbinsConstants.FIRST_BIN_INDEX:]

        else:

            multiplicities = ones
            q2_rebined = q_powder

        self._q_powder_multiplicities = multiplicities
        self._q_powder = q2_rebined

    def get_q_powder_size(self):
        return self._q_powder.size

    def get_q(self):
        return self._q_powder

    def get_multiplicities(self):
        return self._q_powder_multiplicities

    # noinspection PyMethodMayBeStatic
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
        q_point_mesh = np.zeros((total_q_num, dimensions), dtype=AbinsConstants.FLOAT_TYPE)
        n_q = 0

        for i1 in range(i1min, i1max + 1):
            for i2 in range(i2min, i2max + 1):
                for i3 in range(i3min, i3max + 1):
                    q_point_mesh[n_q, :] = [float(i1) / float(AbinsParameters.q_mesh[0]),
                                            float(i2) / float(AbinsParameters.q_mesh[1]),
                                            float(i3) / float(AbinsParameters.q_mesh[2])]
                n_q += 1

        return q_point_mesh

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

            # sigma[freq_num]
            sigma = np.zeros(shape=frequencies.size, dtype=AbinsConstants.FLOAT_TYPE)
            sigma.fill(self._sigma)

            # start[freq_num]
            start = frequencies - AbinsParameters.fwhm * sigma

            # stop[freq_num]
            stop = frequencies + AbinsParameters.fwhm * sigma

            # step[freq_num]
            step = (stop - start) / float((AbinsParameters.pkt_per_peak - 1))

            # matrix_step[freq_num, AbinsParameters.pkt_per_peak]
            matrix_step = np.array([step, ] * AbinsParameters.pkt_per_peak).transpose()

            # matrix_start[freq_num, AbinsParameters.pkt_per_peak]
            matrix_start = np.array([start, ] * AbinsParameters.pkt_per_peak).transpose()

            # broad_freq[freq_num, AbinsParameters.pkt_per_peak]
            broad_freq = (np.arange(0, AbinsParameters.pkt_per_peak) * matrix_step) + matrix_start
            broad_freq[..., -1] = stop

            # sigma_matrix[freq_num, AbinsParameters.pkt_per_peak]
            sigma_matrix = np.array([sigma, ] * AbinsParameters.pkt_per_peak).transpose()

            # frequencies_matrix[freq_num, AbinsParameters.pkt_per_peak]
            frequencies_matrix = np.array([frequencies, ] * AbinsParameters.pkt_per_peak).transpose()

            # gaussian_val[freq_num, AbinsParameters.pkt_per_peak]
            dirac_val = self._dirac_delta(sigma=sigma_matrix, points=broad_freq, center=frequencies_matrix)

            # s_dft_matrix[freq_num, AbinsParameters.pkt_per_peak]
            s_dft_matrix = np.array([s_dft, ] * AbinsParameters.pkt_per_peak).transpose()

            # temp_spectrum[freq_num, AbinsParameters.pkt_per_peak]
            temp_spectrum = s_dft_matrix * dirac_val

            points_freq = np.ravel(broad_freq)
            broadened_spectrum = np.ravel(temp_spectrum)

        return points_freq, broadened_spectrum

    # noinspection PyMethodMayBeStatic
    def _dirac_delta(self, sigma=None, points=None, center=None):

        """
        Dirac delta is implemented as a very narrow Gaussian function.
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma
        return 1.0 / np.sqrt(sigma_factor * np.pi) * np.exp(-(points - center) ** 2 / sigma_factor)

    def get_nspec(self):
        return self._q_powder.size
