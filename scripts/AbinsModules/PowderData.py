import numpy as np

from GeneralData import  GeneralData
import  AbinsConstants

class PowderData(GeneralData):
    """
    Class for storing powder data.
    """
    def __init__(self, num_atoms=None):
        super(PowderData, self).__init__()

        if isinstance(num_atoms, int) and num_atoms > 0:
            self._num_atoms = num_atoms
        else:
            raise ValueError("Invalid value of atoms.")

        self._data = None

    def set(self, items=None):

        self._check_items(items=items)
        self._data = items


    def extract(self):
        self._check_items(items=self._data)
        return self._data


    def _check_items(self, items=None):

         if not isinstance(items, dict):
             raise ValueError("Invalid value. Dictionary with the following entries : %s" % AbinsConstants.all_keywords_powder_data + " was expected")

         if sorted(items.keys()) != sorted(AbinsConstants.all_keywords_powder_data):
            raise ValueError("Invalid structure of the dictionary.")

         if not isinstance(items["a_tensors"], np.ndarray):
            raise ValueError("New value of a_tensor should be a numpy array.")

         if not isinstance(items["b_tensors"], np.ndarray):
            raise ValueError("New value of Debye-Waller factors should be a numpy array.")

         if items["a_tensors"].shape[0] != self._num_atoms:
            raise ValueError("Invalid dimension of a_tensors.")

         if items["b_tensors"].shape[0] != self._num_atoms:
             raise ValueError("Invalid dimension of b_tensors.")

    def __str__(self):
        return "Powder data"