# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
import numbers
from typing import Dict, List, Optional, overload, Union, TypedDict
import re
import numpy as np

import abins
from abins.constants import FLOAT_ID, FLOAT_TYPE


class _AtomData(TypedDict):
    """Item within AtomsData"""

    coord: np.ndarray
    mass: float
    sort: int
    symbol: str


class AtomsData(collections.abc.Sequence):
    def __init__(self, atoms_data: Dict[str, _AtomData]) -> None:
        """Data container for Atomic position information

        AtomsData objects can be iterated, indexed and sliced to obtain
        a dict of data for each atom.

        Args:
            atoms_data: dict of data arranged by atom. Each key is a string of
                the form "atom_0", "atom_1" etc. and each value is a dict with
                signature {"coord": length-3 array,
                           "mass": float,
                           "sort": int,
                           "symbol": str}
                where "coord" is in Angstrom, "mass" is in a.m.u.,
                "sort" is an identifier for symmetry-equivalent sites and
                "symbol" is a chemical element symbol.

        """

        # Make a map matching int indices to atoms_data keys
        test = re.compile(r"^atom_(\d+)$")

        def _get_index_if_atom(label: str) -> Union[int, None]:
            match = test.match(label)
            if match is None:
                return None
            else:
                return int(match.groups()[0])

        all_labels = list(atoms_data.keys())
        # Collect integer keys with corresponding string 'labels'
        # i.e. {0: 'atom_0', 1: 'atom_1', ...}
        atom_labels_by_index = {index: label for index, label in zip(map(_get_index_if_atom, all_labels), all_labels) if index is not None}

        sorted_atom_keys = [atom_labels_by_index[index] for index in sorted(atom_labels_by_index)]
        n_atoms = len(sorted_atom_keys)

        # Check that indices run up from zero with no gaps
        if set(atom_labels_by_index) != set(range(len(atom_labels_by_index))):
            raise ValueError(
                "Missing some atom data. Only these entries were found: \n"
                '{}. key format must be "atom_I" '
                "where I is count starting from zero.".format("\n".join(sorted_atom_keys))
            )

        # Now we can drop these string keys and store as a list with usual indices [{Atom0}, {Atom1}, ...]
        self._data = [self._check_item(atoms_data[key], n_atoms=n_atoms) for key in sorted_atom_keys]

    @staticmethod
    def _check_item(item: _AtomData, n_atoms: Optional[int] = None) -> _AtomData:
        """
        Check a dict of atom data is suitable for use as item in AtomsData

        Args:
            item: element to be added
            n_atoms: Number of atoms in data. If provided, check that "sort" value is not higher than expected.

        Returns:
            The input item
        """

        if not isinstance(item, dict):
            raise ValueError("Every element of AtomsData should be a dictionary.")

        if not sorted(item.keys()) == sorted(abins.constants.ALL_KEYWORDS_ATOMS_DATA):
            raise ValueError("Invalid structure of the dictionary to be added.")

        # "symbol"
        if (symbol_raw := item["symbol"]) not in abins.constants.ALL_SYMBOLS:
            # Check if symbol was loaded as type bytes
            if isinstance(symbol_raw, bytes):
                symbol = symbol_raw.decode("utf-8")
            else:
                symbol = symbol_raw
                assert isinstance(symbol, str)

            # Symbols from CASTEP may have subtypes e.g. H:D
            if ":" in symbol:
                symbol = symbol.split(":")[0]

            # Final check that value is now valid
            if symbol in abins.constants.ALL_SYMBOLS:
                item["symbol"] = symbol
            else:
                raise ValueError(f"Invalid value of symbol: {item['symbol']}.")

        # "coord"
        coord = item["coord"]
        if not isinstance(coord, np.ndarray):
            raise ValueError("Coordinates of an atom should have a form of a numpy array.")
        if len(coord.shape) != 1:
            raise ValueError("Coordinates should have a form of 1D numpy array.")
        if coord.shape[0] != 3:
            raise ValueError("Coordinates should have a form of numpy array with three elements.")
        if coord.dtype.num != FLOAT_ID:
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

    @overload
    def __getitem__(self, item: int) -> _AtomData: ...

    @overload
    def __getitem__(self, item: slice) -> List[_AtomData]: ...

    def __getitem__(self, item):
        return self._data[item]

    def extract(self):
        # For compatibility, regenerate the dict format on-the-fly
        return {f"atom_{i}": item for i, item in enumerate(self._data)}

    class JSONableAtomData(TypedDict):
        """JSON-friendly representation of an AtomsData entry"""

        coord: List[float]
        mass: float
        sort: int
        symbol: str

    JSONableData = Dict[str, "AtomsData.JSONableAtomData"]

    def to_dict(self) -> "AtomsData.JSONableData":
        """Get a JSON-compatible representation of the data"""
        data: "AtomsData.JSONableData"
        data = {
            f"atom_{i}": {
                "coord": item["coord"].tolist(),
                "mass": float(item["mass"]),
                "sort": int(item["sort"]),
                "symbol": str(item["symbol"]),
            }
            for i, item in enumerate(self._data)
        }
        return data

    @staticmethod
    def from_dict(data: "AtomsData.JSONableData") -> "AtomsData":
        """Construct AtomsData from JSON-compatible dictionary

        Args:
            data: Unlike the main constructor AtomsData(), this data should
                be in JSON-compatible form: i.e. the length-3 numpy arrays are
                replaced with length-3 List[float]. Usually this data will be
                obtained from an external file or from the AtomsData.to_dict()
                method.

        """
        atoms_data = {}  # type: Dict[str, _AtomData]

        for atom_key, atom_data in data.items():
            atoms_data[atom_key] = {
                "coord": np.asarray(atom_data["coord"], dtype=FLOAT_TYPE),
                "mass": atom_data["mass"],
                "sort": atom_data["sort"],
                "symbol": atom_data["symbol"],
            }

        return AtomsData(atoms_data)

    def __str__(self):
        return "Atoms data"
