class Instrument(object):

    _name  = None

    def calculate_q_powder(self, overtones=None):
        """
        Calculates q vectors.

        @param overtones: True if overtones should be included in calculations, otherwise False
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


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, start=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        @param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero

       """
        return None


    def __str__(self):
        return self._name


    def getName(self):
        return self._name
