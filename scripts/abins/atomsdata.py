# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
import numbers
from typing import Any, Dict, Sequence

>>>>>>> Abins AtomsData: Rework to be immutable
import numpy as np
import abins


class AtomsData:
    def __init__(self, atoms_data: Sequence[Dict[str, Any]]):
        self._data = {}

        test = re.compile(r'^atom_(\d+)$')
        for key, atom_data in atoms_data.items():
            match = test.match(key)
            if match is not None:
                atom_index = int(match.groups()[0])
                self._check_item(atom_data)
                self._data.update({f'atom_{atom_index}': atom_data})

        expected_keys = [f'atom_{i}' for i in range(len(self._data))]
        if set(expected_keys) != set(self._data.keys()):
            raise ValueError('Missing some atom data. Only these entries were found: {}. key format must be "atom_I" '
                             'where I is count starting from zero.'.format(' '.join(sorted(self._data.keys()))))

    @staticmethod
    def _check_item(item):
        """
        Raise an error if Atoms data item is unsuitable

        :param item: element to be added
        """

        if not isinstance(item, dict):
            raise ValueError("Every element of AtomsData should be a dictionary.")

        if not sorted(item.keys()) == sorted(abins.constants.ALL_KEYWORDS_ATOMS_DATA):
            raise ValueError("Invalid structure of the dictionary to be added.")

        # "symbol"
        if not item["symbol"] in abins.constants.ALL_SYMBOLS:
            raise ValueError("Invalid value of symbol.")

        # "coord"
        coord = item["coord"]
        if not isinstance(coord, np.ndarray):
            raise ValueError("Coordinates of an atom should have a form of a numpy array.")
        if len(coord.shape) != 1:
            raise ValueError("Coordinates should have a form of 1D numpy array.")
        if coord.shape[0] != 3:
            raise ValueError("Coordinates should have a form of numpy array with three elements.")
        if coord.dtype.num != abins.constants.FLOAT_ID:
            raise ValueError("Coordinates array should have real float dtype.")

        # "sort"
        sort = item["sort"]
        if not isinstance(sort, numbers.Integral):
            raise ValueError("Parameter 'sort' should be integer.")

        if sort < 0:
            raise ValueError("Parameter 'sort' cannot be negative.")

        # "mass"
        mass = item["mass"]
        if not isinstance(mass, numbers.Real):
            raise ValueError("Mass of atom should be a real number.")
        if mass < 0:
            raise ValueError("Mass of atom cannot be negative.")

    def extract(self):
        return self._data

    def __str__(self):
        return "Atoms data"
