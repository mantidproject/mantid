import numpy as np
from GeneralData import GeneralData

class QData(GeneralData):
    """
    Class for storing Q data.
    """
    def __init__(self, quantum_order_events_num=None):
        """
        @param quantum_order_events_num: True if overtones should be included in calculations, otherwise False
        """
        super(QData, self).__init__()
        if isinstance(quantum_order_events_num, int):
            self._quantum_order_events_num = quantum_order_events_num
        else:
            raise ValueError("Invalid value of quantum order events.")

    def set(self, items=None):

        if isinstance(items, dict):
            self._data = items
        else:
            raise ValueError("Improper format of Q data. Dictionary is expected.")

    def extract(self):

        return self._data

    def __str__(self):
        return "Q vectors data"







