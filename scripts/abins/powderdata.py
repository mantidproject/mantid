# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abins.constants import ALL_KEYWORDS_POWDER_DATA, GAMMA_POINT


class PowderData:
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
            raise ValueError("Invalid value. Dictionary with the following entries : %s" %
                             ALL_KEYWORDS_POWDER_DATA + " was expected.")

        if sorted(items.keys()) != sorted(ALL_KEYWORDS_POWDER_DATA):
            raise ValueError("Invalid structure of the dictionary.")

        if not isinstance(items["a_tensors"], dict):
            raise ValueError("New value of a_tensor should be a dictionary.")

        if not isinstance(items["b_tensors"], dict):
            raise ValueError("New value of Debye-Waller factors should be a dictionary.")

        if items["a_tensors"][GAMMA_POINT].shape[0] != self._num_atoms:
            raise ValueError("Invalid dimension of a_tensors.")

        if items["b_tensors"][GAMMA_POINT].shape[0] != self._num_atoms:
            raise ValueError("Invalid dimension of b_tensors.")

    def __str__(self):
        return "Powder data"
