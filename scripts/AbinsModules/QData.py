import numpy as np
from GeneralData import GeneralData

class QData(GeneralData):
    """
    Class for storing Q data.
    """
    def __init__(self, frequency_dependence=None):

        super(QData, self).__init__()

        if (isinstance(frequency_dependence, bool) or
            isinstance(frequency_dependence, np.bool_) or
            isinstance(frequency_dependence, np.bool)):
            self._frequency_dependence = frequency_dependence
        else:
            raise ValueError("Invalid value of parameter frequency_dependence (value: True/False is expected). ")

        self._num_k = None


    def _append(self, item=None):
        """
        Appends one item to the collection of Q data.
        @param item: item to be added
        """
        if not (isinstance(item, np.ndarray)):
            raise ValueError("Invalid value of item to be added to a collection of Q items.")

        self._data.append(item)


    def set(self, items=None):

        if isinstance(items, np.ndarray):
            self._data = items

        if isinstance(items, list):
            self._data = []
            for item in items:
                self._append(item=item)


    def set_k(self, k):
        """
        Sets number of k-points
        @param k: number of k-point
        """
        if isinstance(k, int) and k > 0 :
            self._num_k = k
        else:
            raise ValueError("Invalid number of k-points.")


    def extract(self):

        if isinstance(self._data, list):
            self._data = np.asarray(self._data)

        if not len(self._data.shape) == 2:
            raise ValueError("Improper format of Q data. Two dimentional array is expected.")


        if self._frequency_dependence:

            if self._num_k != self._data.shape[0]:
                raise ValueError("Inconsistent number of k-points and size of data.")

        else:

            if self._data.shape[1] != 3:
                raise ValueError("The second dimension of Q data is expected to be 3.")

        return self._data


    def __str__(self):
        return "Q vectors data"







