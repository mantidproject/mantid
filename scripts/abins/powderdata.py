# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numbers
from typing import Any, Dict, Optional
from abins.constants import ALL_KEYWORDS_POWDER_DATA, GAMMA_POINT

PowderItems = Dict[str, Dict[str, Any]]


class PowderData:
    """
    Data container for tensors used in analytic powder-averaging model

    :param items: Tensor data (dict containing 'a_tensors', 'b_tensors', which
        are dicts of data by k-point)
    :param num_atoms: Expected number of atoms in tensor data. If provided,
        this value is used for sanity-checking

    """
    def __init__(self, items: PowderItems,
                 num_atoms: Optional[int] = None):

        if isinstance(num_atoms, numbers.Integral) and num_atoms > 0:
            self._num_atoms = int(num_atoms)  # type: int
        else:
            raise ValueError("Invalid value of atoms.")

        self._check_items(items)
        self._data = items  # type: PowderItems

    def extract(self) -> PowderItems:
        """Get tensor data as dict"""
        self._check_items(self._data)
        return self._data

    def _check_items(self, items: PowderItems) -> None:

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

    def __str__(self) -> str:
        return "Tensor data for analytic powder averaging"
