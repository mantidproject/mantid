from AbinsModules import Constants

class InstrumentInterface(object):

    _name  = None

    def calculate_q(self, frequencies=None):
        """

        @param frequencies:  frequencies for which Q data should be calculated
        """
        return None


    def calculate_resolution_function(self, frequencies=None):
        """
        Calculates resolution function for the particular instrument.
        @param frequencies:   frequencies for which resolution function should be calculated

       """
        return None

