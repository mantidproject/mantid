from InstrumentInterface import  InstrumentInterface
from AbinsModules import Constants

class ToscaInstrument(InstrumentInterface):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name

    def calculate_q(self, frequencies=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        return frequencies * frequencies * Constants.TOSCA_constant


    def calculate_resolution_function(self, frequencies=None):
        """
        Calculates resolution function for the particular instrument.
        @param frequencies:   frequencies for which resolution function should be calculated

       """
        return None