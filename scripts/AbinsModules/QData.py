import numpy as np
from GeneralData import GeneralData

class QData(GeneralData):
    """
    Class for storing Q data.
    """
    def __init__(self, overtones=None):
        """
        @param overtones: True if overtones should be included in calculations, otherwise False
        """
        super(QData, self).__init__()
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







