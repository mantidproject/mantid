# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
from typing import List, NamedTuple, overload
import numpy as np

from abins.constants import (COMPLEX_ID, FLOAT_ID, GAMMA_POINT, SMALL_K)


class KpointData(NamedTuple):
    """Vibrational frequency / displacement data at a particular k-point"""
    k: np.ndarray
    weight: float
    frequencies: np.ndarray
    atomic_displacements: np.ndarray


class KpointsData(collections.abc.Sequence):
    """Class storing atomic frequencies and displacements at specific k-points

    "weights" - weights of all k-points; weights.shape == (num_k,);

    "k_vectors"  - k_vectors of all k-points;  k_vectors.shape == (num_k, 3)

    "frequencies" - frequencies for all k-points; frequencies.shape == (num_k, num_freq)

    "atomic_displacements" - atomic displacements for all k-points;
                             atomic_displacements.shape == (num_k, num_atoms, num_freq, 3)

    "unit_cell" - lattice vectors (use zeros for open boundary conditions);
                  unit_cell.shape == (3, 3)

    """
    def __init__(self, *, frequencies: np.ndarray, atomic_displacements: np.ndarray,
                 weights: np.ndarray, k_vectors: np.ndarray, unit_cell: np.ndarray) -> None:
        super().__init__()

        self._data = {}
        dim = 3

        for arg in (frequencies, atomic_displacements, weights, k_vectors, unit_cell):
            if not isinstance(arg, np.ndarray):
                raise TypeError("All arguments to KpointsData should be numpy arrays")

        # unit_cell
        if not (unit_cell.shape == (dim, dim)
                and unit_cell.dtype.num == FLOAT_ID):
            raise ValueError("Invalid values of unit cell vectors.")
        self.unit_cell = unit_cell

        # weights
        num_k = weights.size
        if not (weights.dtype.num == FLOAT_ID
                and np.allclose(weights, weights[weights >= 0])):
            raise ValueError("Invalid value of weights.")
        self._weights = weights

        # k_vectors
        if not (k_vectors.shape == (num_k, dim)
                and k_vectors.dtype.num == FLOAT_ID):
            raise ValueError("Invalid value of k_vectors.")
        self._k_vectors = k_vectors

        # frequencies
        num_freq = frequencies.shape[1]
        if not (frequencies.shape == (num_k, num_freq)
                and frequencies.dtype.num == FLOAT_ID):
            raise ValueError("Invalid value of frequencies.")
        self._frequencies = frequencies

        # atomic_displacements
        if len(atomic_displacements.shape) != 4:
            raise ValueError("atomic_displacements should have four dimensions")
        num_atoms = atomic_displacements.shape[1]

        if not (atomic_displacements.shape == (weights.size, num_atoms, num_freq, dim)
                and atomic_displacements.dtype.num == COMPLEX_ID):
            raise ValueError("Invalid value of atomic_displacements.")
        self._atomic_displacements = atomic_displacements

    @staticmethod
    def _array_to_dict(array, string_key=False):
        if string_key:
            return {str(i): row for i, row in enumerate(array)}
        else:
            return {i: row for i, row in enumerate(array)}

    def get_gamma_point_data(self):
        """
        Extracts k points data only for Gamma point.
        :returns: dictionary with data only for Gamma point
        """
        gamma_pkt_index = -1
        k_vectors = self._array_to_dict(self._k_vectors)

        # look for index of Gamma point
        for k_index, k in k_vectors.items():
            if np.linalg.norm(k) < SMALL_K:
                gamma_pkt_index = k_index
                break
        else:
            raise ValueError("Gamma point not found.")

        k_points = {"weights": {GAMMA_POINT: self._data["weights"][gamma_pkt_index]},
                    "k_vectors": {GAMMA_POINT: self._data["k_vectors"][gamma_pkt_index]},
                    "frequencies": {GAMMA_POINT: self._data["frequencies"][gamma_pkt_index]},
                    "atomic_displacements": {GAMMA_POINT: self._data["atomic_displacements"][gamma_pkt_index]},
                    "unit_cell": self.unit_cell}

        return k_points

    def extract(self):
        extracted = {"unit_cell": self.unit_cell,
                     "weights": self._array_to_dict(self._weights, string_key=True),
                     "k_vectors": self._array_to_dict(self._k_vectors, string_key=True),
                     "frequencies": self._array_to_dict(self._frequencies, string_key=True),
                     "atomic_displacements": self._array_to_dict(self._atomic_displacements, string_key=True)}
        return extracted

    def __str__(self):
        return "K-points data"

    def __len__(self):
        return self._weights.size

    @overload  # noqa F811
    def __getitem__(self, item: int) -> KpointData:
        ...

    @overload  # noqa F811
    def __getitem__(self, item: slice) -> List[KpointData]:
        ...

    def __getitem__(self, item):  # noqa F811
        if isinstance(item, int):
            return KpointData(self._k_vectors[item],
                              self._weights[item],
                              self._frequencies[item],
                              self._atomic_displacements[item])
        elif isinstance(item, slice):
            return [self[i] for i in range(len(self))[item]]

        return self._data[item]
