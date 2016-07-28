class Instrument(object):

    _name  = None

    def calculate_q(self, frequencies=None):
        """

        @param frequencies:  frequencies for which Q data should be calculated (frequencies in Hartree atomic units)
        """
        return None


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        @param frequencies:   frequencies for which resolution function should be calculated (frequencies in Hartree atomic units)
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak

       """
        return None

    def produce_abscissa(self, frequencies=None, points_per_peak=None):
        """
        Creates abscissa for convoluted spectrum.
        @param frequencies: DFT frequencies for which frequencies which correspond to broadened spectrum should be regenerated (frequencies in Hartree atomic units)
        @param points_per_peak:  number of points for each peak of broadened spectrum
        @return: abscissa for convoluted S
        """
        return None
