class Instrument(object):

    _name  = None

    def calculate_q_powder(self):
        """
        Calculates q vectors.
        @return:  numpy array with Q data
        """
        ""
        return None


    def collect_K_data(self, k_points_data=None):
        """
        Collects k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """
        return None


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None, start=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        @param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero

       """
        return None


    def produce_abscissa(self, frequencies=None, points_per_peak=None, start=None):
        """
        Creates abscissa for convoluted spectrum.
        @param frequencies: DFT frequencies for which frequencies which correspond to broadened spectrum should be regenerated (frequencies in cm-1)
        @param points_per_peak:  number of points for each peak of broadened spectrum
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        @return: abscissa for convoluted S
        """
        return None


    def __str__(self):
        return self._name


    def getName(self):
        return self._name
