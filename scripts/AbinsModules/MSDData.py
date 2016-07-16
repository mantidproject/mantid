
import numpy as np

from GeneralData import  GeneralData
import  Constants

class MSDData(GeneralData):
    """
    Class for storing mean square displacements.
    """
    def __init__(self, temperature=None, num_atoms=None):
        super(MSDData, self).__init__()
        if not ((isinstance(temperature, float) or isinstance(temperature, int)) and temperature > 0):
            raise ValueError("Invalid value of temperature.")
        self._temperature = temperature

        if isinstance(num_atoms, int) and num_atoms > 0:
            self._num_atoms = num_atoms
        else:
            raise ValueError("Invalid value of atoms.")

        self._data = np.array(self._num_atoms, dtype=Constants.float_type)


    def _append(self, num_atom=None, msd_atom=None):
        if not (isinstance(num_atom, int) and 0 <= num_atom < self._num_atoms):
            raise ValueError("Invalid number of atom.")

        if isinstance(msd_atom, float):
            self._data[num_atom] = msd_atom
        else:
            raise ValueError("Invalid value of mean square displacement.")


    def set(self, items=None):

        if not isinstance(items, np.ndarray):
            raise ValueError("New value of mean square displacements should be a numpy array.")
        if items.shape != (self._num_atoms,):
            raise ValueError("Invalid size of mean square displacements.")

        self._data = items


    def extract(self):
        if isinstance(self._data, np.ndarray) and self._data.shape == (self._num_atoms,):
            return self._data
        else:
            raise ValueError("Mean square displacements are not valid.")