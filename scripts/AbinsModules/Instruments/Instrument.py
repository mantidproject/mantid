from AbinsModules import Constants

class Instrument(object):

    _name  = None

    def calculate_q(self, frequencies=None):
        """

        @param frequencies:  frequencies for which Q data should be calculated
        """
        return None


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        @param frequencies:   frequencies for which resolution function should be calculated
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak

       """
        return None

