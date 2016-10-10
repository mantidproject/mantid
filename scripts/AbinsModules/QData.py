import numpy as np
from GeneralData import GeneralData

class QData(GeneralData):
    """
    Class for storing Q data.
    """
    def __init__(self, num_k=None, overtones=None):
        """
        @param num_k: number of k-points
        @param overtones: True if overtones should be included in calculations, otherwise False
        """
        super(QData, self).__init__()
        if isinstance(num_k, int) and num_k > 0 :
            self._num_k = num_k
        else:
            raise ValueError("Invalid number of k-points.")

        if isinstance(overtones, bool):
            self._overtones = overtones
        else:
            raise ValueError("Invalid value of overtones. Expected values are: True, False ")


    def set(self, items=None):

        if isinstance(items, dict):
            self._data = items
        else:
            raise ValueError("Improper format of Q data. Dictionary is expected.")


    def extract(self):

        return self._data


    def __str__(self):
        return "Q vectors data"







