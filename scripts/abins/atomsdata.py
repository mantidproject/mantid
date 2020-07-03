# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
import numbers
from typing import Any, Dict, List, Optional, overload, Union
import re
import numpy as np
import abins


class AtomsData(collections.abc.Sequence):
    def __init__(self, atoms_data: Dict[str, Dict[str, Any]]) -> None:

        # Make a map matching int indices to atoms_data keys
        test = re.compile(r'^atom_(\d+)$')
        def _get_index_if_atom(label: str) -> Union[int, None]:
            match = test.match(label)
            if match is None:
                return None
            else:
                return int(match.groups()[0])

        all_labels = list(atoms_data.keys())
        # Collect integer keys with corresponding string 'labels'
        # i.e. {0: 'atom_0', 1: 'atom_1', ...}
        atom_labels_by_index = {index: label
                                for index, label in zip(map(_get_index_if_atom, all_labels),
                                                        all_labels)
                                if index is not None}

        sorted_atom_keys = [atom_labels_by_index[index] for index in sorted(atom_labels_by_index)]
        n_atoms = len(sorted_atom_keys)

        # Check that indices run up from zero with no gaps
        if set(atom_labels_by_index) != set(range(len(atom_labels_by_index))):
            raise ValueError('Missing some atom data. Only these entries were found: \n'
                             '{}. key format must be "atom_I" '
                             'where I is count starting from zero.'.format('\n'.join(sorted_atom_keys)))

        # Now we can drop these string keys and store as a list with usual indices [{Atom0}, {Atom1}, ...]
        self._data = [self._check_item(atoms_data[key], n_atoms=n_atoms) for key in sorted_atom_keys]

    @staticmethod
    def _check_item(item: Dict[str, Any], n_atoms: Optional[int] = None) -> Dict[str, Any]:
        """
        Raise an error if Atoms data item is unsuitable

        :param item: element to be added
        :param n_atoms: Number of atoms in data. If provided, check that "sort" value is not higher than expected.
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

        if n_atoms is not None and (sort + 1) > n_atoms:
            raise ValueError("Parameter 'sort' should not exceed atom indices")

        # "mass"
        mass = item["mass"]
        if not isinstance(mass, numbers.Real):
            raise ValueError("Mass of atom should be a real number.")
        if mass < 0:
            raise ValueError("Mass of atom cannot be negative.")

        return item

    def __len__(self) -> int:
        return len(self._data)

    @overload  # noqa F811
    def __getitem__(self, item: int) -> Dict[str, Any]:
        ...

    @overload  # noqa F811
    def __getitem__(self, item: slice) -> List[Dict[str, Any]]:
        ...

    def __getitem__(self, item):  # noqa F811
        return self._data[item]

    def extract(self):
        # For compatibility, regenerate the dict format on-the-fly
        return {f'atom_{i}': item for i, item in enumerate(self._data)}

    def __str__(self):
        return "Atoms data"
