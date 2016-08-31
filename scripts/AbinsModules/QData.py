import numpy as np
from GeneralData import GeneralData

class QData(GeneralData):
    """
    Class for storing Q data.
    """
    def __init__(self, num_k=None):

        super(QData, self).__init__()
        if isinstance(num_k, int) and num_k > 0 :
            self._num_k = num_k
        else:
            raise ValueError("Invalid number of k-points.")


    def set(self, items=None):

        if isinstance(items, np.ndarray):
            self._data = items


    def extract(self):

        if not len(self._data.shape) == 2:
            raise ValueError("Improper format of Q data. Two dimentional array is expected.")

        if self._num_k != self._data.shape[0]:
            raise ValueError("Inconsistent number of k-points and size of data.")

        return self._data


    def __str__(self):
        return "Q vectors data"







