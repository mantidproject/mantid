# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, Optional

import numpy as np
from pydantic import validate_call
from pydantic.types import PositiveInt

PowderDict = Dict[str, Dict[int, np.ndarray]]


class PowderData:
    """
    Data container for tensors used in analytic powder-averaging model

    :param a_tensors: dict of total displacement tensors, indexed by integer
        k-point identities
    :param b_tensors: dict of mode-by-mode tensors, indexed by integer k-point
        identities
    :param frequencies: frequencies corresponding to data in b_tensors; usually
        this has already been pruned to remove imaginary modes.

    :param n_plus_1: Bose occupation <n+1> values corresponding to
        modes. b_tensors should already be scaled by <n+1>, a_tensors by <2n+1>

    :param num_atoms: Expected number of atoms in tensor data. If provided,
        this value is used for sanity-checking
    """

    @validate_call(config=dict(arbitrary_types_allowed=True, strict=True))
    def __init__(
        self,
        *,
        a_tensors: Dict[int, np.ndarray],
        b_tensors: Dict[int, np.ndarray],
        frequencies: Dict[int, np.ndarray],
        n_plus_1: Dict[int, np.ndarray],
        num_atoms: Optional[PositiveInt] = None,
    ):
        self._data = {"a_tensors": a_tensors, "b_tensors": b_tensors, "frequencies": frequencies, "n_plus_1": n_plus_1}  # type: PowderDict

        self._num_atoms = num_atoms

        self._check_data()

        self._a_traces = {}

    def get_a_tensors(self) -> Dict[int, np.ndarray]:
        return self._data["a_tensors"]

    def get_b_tensors(self) -> Dict[int, np.ndarray]:
        return self._data["b_tensors"]

    def get_a_traces(self, k_index):
        if k_index not in self._a_traces:
            self._calculate_a_traces(k_index)
        return self._a_traces[k_index]

    def _calculate_a_traces(self, k_index):
        self._a_traces[k_index] = np.trace(a=self.get_a_tensors()[k_index], axis1=1, axis2=2)

    def get_b_traces(self, k_index):
        return np.trace(a=self.get_b_tensors()[k_index], axis1=2, axis2=3)

    def get_frequencies(self) -> np.ndarray:
        return self._data["frequencies"]

    def get_n_plus_1(self, k_index):
        return self._data["n_plus_1"][k_index]

    def extract(self) -> PowderDict:
        """Get tensor data as dict"""
        return {key: {str(k): array for k, array in data.items()} for key, data in self._data.items()}

    @classmethod
    def from_extracted(cls, dct: PowderDict, num_atoms: Optional[int] = None):
        """Reconstruct a PowderData object from the extracted dictionary representation"""
        a_tensors = {int(k_index): data for k_index, data in dct["a_tensors"].items()}
        b_tensors = {int(k_index): data for k_index, data in dct["b_tensors"].items()}
        frequencies = {int(k_index): data for k_index, data in dct["frequencies"].items()}
        n_plus_1 = {int(k_index): data for k_index, data in dct["n_plus_1"].items()}
        return cls(a_tensors=a_tensors, b_tensors=b_tensors, frequencies=frequencies, num_atoms=num_atoms, n_plus_1=n_plus_1)

    def _check_data(self) -> None:
        if self._num_atoms is not None:
            for _, tensor in self.get_a_tensors().items():
                if tensor.shape[0] != self._num_atoms:
                    raise ValueError("Invalid dimension of a_tensors.")

            for _, tensor in self.get_b_tensors().items():
                if tensor.shape[0] != self._num_atoms:
                    raise ValueError("Invalid dimension of b_tensors.")

        if self.get_frequencies().keys() != self.get_a_tensors().keys():
            raise ValueError("Frequency data does not cover same number of kpts as a_tensors")

        if self.get_frequencies().keys() != self.get_b_tensors().keys():
            raise ValueError("Frequency data does not cover same number of kpts as b_tensors")

        for k, frequency_set in self.get_frequencies().items():
            if frequency_set.size != self.get_b_tensors()[k].shape[1]:
                raise ValueError(f"Number of frequencies does not match shape of b_tensors at k-point {k}")

    def __str__(self) -> str:
        return "Tensor data for analytic powder averaging"
