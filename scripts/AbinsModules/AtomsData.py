import numpy as np
from GeneralData import GeneralData
import AbinsParameters

class AtomsDaTa(GeneralData):

    def __init__(self, num_atoms=None):
        super(AtomsDaTa, self).__init__()
        if not isinstance(num_atoms, int):
            raise ValueError("Invalid number of atoms.")
        if num_atoms< 0:
            raise ValueError("Number of atoms cannot be negative.")
        self._num_atoms = num_atoms


    def _append(self, item=None):
        """
        Adds one elements to the collection of atoms data.
        @param item: element to be added
        """

        if not isinstance(item, dict):
            raise  ValueError("Every element of AtomsData has  a form of the dictionary.")

        if not sorted(item.keys()) == sorted(AbinsParameters.all_keywords_atoms_data):
            raise ValueError("Invalid structure of the dictionary to be added.")

        # "symbol"
        if not item["symbol"] in AbinsParameters.all_symbols:
            raise ValueError("Invalid value of symbol.")

        # "fract_coord"
        fract_coord = item["fract_coord"]
        if not isinstance(fract_coord, np.ndarray):
            raise ValueError("Coordinates of an atom should have a form of a numpy array.")
        if len(fract_coord.shape)  != 1:
            raise ValueError("Coordinates should have a form of 1D numpy array.")
        if fract_coord.shape[0] != 3:
            raise ValueError("Coordinates should have a form of numpy array with three elements.")
        if fract_coord.dtype.num !=  AbinsParameters.float_id:
            raise ValueError("All coordinates should be real numbers.")

        # "atom"
        atom = item["atom"]
        if not isinstance(atom, int):
            raise ValueError("Number of atom should be integer.")
        if atom < 0:
            raise ValueError("Number of atom cannot be negative.")
        if atom  >= self._num_atoms: # here = because we count from 0
            raise ValueError("Number of atoms cannot be larger than the total number of atoms.")

        # "sort"
        sort = item["sort"]
        if not isinstance(sort, int):
            raise ValueError("Parameter sort  should be integer.")
        if sort < 0:
            raise ValueError("Parameter sort cannot be negative.")
        if sort  >= self._num_atoms: # here = because we count from 0
            raise ValueError("Parameter sort cannot be larger than the total number of atoms.")

        # "mass"
        mass = item["mass"]
        if not isinstance(mass, float):
            raise ValueError("Mass of atom should be a real number.")
        if mass < 0:
            raise ValueError("Mass of atom cannot be negative.")

        self._data.append(item)


    def set(self, items=None):


        if len(items) != self._num_atoms:
            raise ValueError("Inconsistent size of new data and number of atoms. (%s != %s)"%(len(items), self._num_atoms))

        if isinstance(items, list):
            self._data = []
            for item in items:
                self._append(item=item)


    def extract(self):
        if len(self._data) == self._num_atoms:
            return self._data
        else:
            raise ValueError("Size of AtomsData and number of atoms is inconsistent.")


    def __str__(self):
        return "Atoms data"

