# noinspection PyPep8Naming
class Instrument(object):

    _name = None

    def calculate_q_powder(self, quantum_order_events_num=None):
        """
        Calculates q vectors.

        @param quantum_order_events_num: True if combinations should be calculated, otherwise False
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

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        @param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT

       """
        return None

    def __str__(self):
        return self._name

    def getName(self):
        return self._name
