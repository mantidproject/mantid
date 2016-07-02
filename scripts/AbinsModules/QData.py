import numpy as np
from GeneralData import GeneralData
import  Constants

class QData(GeneralData):
    """

    """
    def __init__(self, q_format=None):

        super(QData, self).__init__()

        if q_format not in Constants.all_Q_formats:
            raise ValueError("Invalid format of Q vectors!")
        self._q_format = q_format

    def set(self, items=None):
        """
        Sets a new collections of Q items.
        @param items: list with Q items (vectors or scalars)
        """

        if not isinstance(items, np.ndarray):
            raise ValueError("Invalid value of items to be added to collection of Q items!")
        if self._q_format == "scalars":
            if len(items.shape) != 1 or not all([isinstance(items[el], float) for el in range(items.shape[0])]):
                raise ValueError("Q data in the form of scalars. Each entry should be a scalar!")
        if self._q_format == "vectors":
            if len(items.shape) != 2:
                raise ValueError("Q data in the form of vectors. Each entry should be a vector!")
            for el1 in range(items.shape[0]):
                if items.shape[1] != 3:
                    raise ValueError("Q data in the form of vectors. Each entry should be numpy array with three elements!")

                if not all([isinstance(items[el1][el2],float) for el2 in range(items.shape[1])]):
                    raise ValueError("Q data in the form of vectors. Each entry should be numpy array with three float elements!")

        self._data = items


    def extract(self):
        """
        Returns Q data as a numpy array.
        @return: collection of Q items.
        """
        return self._data




