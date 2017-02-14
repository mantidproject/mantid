from __future__ import (absolute_import, division, print_function)
import numpy as np
import six
import AbinsModules


class KpointsData(AbinsModules.GeneralData):
    """
    Class for storing k-points data. The data is arranged as a dictionary.
    The dictionary has the following form:

    data = {"frequencies": numpy.array,
            "atomic_displacements: numpy.array,
            "weights": numpy.array,
            "k_vectors": numpy.array}


    Each entry in the dictionary corresponds to all k-points. Each item in the dictionary is a numpy array. The meaning
    of keys in the dictionary is as follows:

    "weights" - weights of all k-points; weights.shape == (self._num_k,);

    "k_vectors"  - k_vectors of all k-points;  k_vectors.shape == (self._num_k, 3)

    "frequencies" - frequencies for all k-points; frequencies.shape == (self._num_k, self._num_freq)

    "atomic_displacements - atomic displacements for all k-points;
                            atomic_displacements.shape == (self._num_k, self._num_atoms, self._num_freq, 3)


    """

    def __init__(self, num_k=None, num_atoms=None):
        """
        @param num_k: total number of k-points (int)
        @param num_atoms: total number of atoms in the unit cell (int)
        """
        super(KpointsData, self).__init__()
        dim = 3  # number of coordinates

        if isinstance(num_k, six.integer_types) and num_k > 0:
            self._num_k = num_k
        else:
            raise ValueError("Invalid number of k-points.")

        if isinstance(num_atoms, six.integer_types) and num_atoms > 0:
            self._num_freq = dim * num_atoms  # number of phonons for one k-point
            self._num_atoms = num_atoms  # number of displacements for one k-point
        else:
            raise ValueError("Invalid number of atoms.")

        self._data = {}

    def set(self, items=None):

        if not isinstance(items, dict):
            raise ValueError("New value of KpointsData should be a dictionary.")

        if not sorted(items.keys()) == sorted(AbinsModules.AbinsConstants.ALL_KEYWORDS_K_DATA):
            raise ValueError("Invalid structure of the dictionary.")

        dim = 3
        #  "weights"
        weights = items["weights"]

        if (isinstance(weights, np.ndarray) and
           weights.shape == (self._num_k,) and
           weights.dtype.num == AbinsModules.AbinsConstants.FLOAT_ID and
           np.allclose(weights, weights[weights >= 0])):

            self._data["weights"] = weights
        else:
            raise ValueError("Invalid value of weights.")

        #  "k_vectors"
        k_vectors = items["k_vectors"]

        if (isinstance(k_vectors, np.ndarray) and
           k_vectors.shape == (self._num_k, dim) and
           k_vectors.dtype.num == AbinsModules.AbinsConstants.FLOAT_ID):

            self._data["k_vectors"] = k_vectors
        else:
            raise ValueError("Invalid value of k_vectors.")

        #  "frequencies"
        frequencies = items["frequencies"]

        if (isinstance(frequencies, np.ndarray) and
           frequencies.shape == (self._num_k, self._num_freq) and
           frequencies.dtype.num == AbinsModules.AbinsConstants.FLOAT_ID):

            self._data["frequencies"] = frequencies
        else:
            raise ValueError("Invalid value of frequencies.")

        #  "atomic_displacements"
        atomic_displacements = items["atomic_displacements"]

        if (isinstance(atomic_displacements, np.ndarray) and
           atomic_displacements.shape == (self._num_k, self._num_atoms, self._num_freq, dim) and
           atomic_displacements.dtype.num == AbinsModules.AbinsConstants.COMPLEX_ID):

            self._data["atomic_displacements"] = atomic_displacements
        else:
            raise ValueError("Invalid value of atomic_displacements.")

    def extract(self):

        return self._data

    def __str__(self):
        return "K-points data"
