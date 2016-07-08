import  numpy as np

# ABINS modules
from GeneralData import GeneralData
import Constants


class DwData(GeneralData):
    """
    Data structure for Debye-Waller coefficients.
    """
    def __init__(self, temperature=None, num_atoms=None):
        """
        @param temperature:  temperature in K
        @param num_atom: number of atoms in the unit cell
        """
        super(DwData, self).__init__()

        if isinstance(temperature, int) and temperature > 0:
            self._temperature = temperature
        else:
            raise ValueError("Improper value of temperature.")

        if isinstance(num_atoms, int) and num_atoms > 0:
            self._num_atoms = num_atoms
        else:
            raise ValueError("Improper number of atoms.")

        self._data = np.zeros((self._num_atoms, 3, 3), dtype=Constants.floats_type)


    def append(self, item=None, num_atom=None):
        """
        Appends DW tensor for one atom.

        @param item:  DW tensor for one atom.
        @param num_atom: number of atom

        """
        if not isinstance(num_atom, int):
            raise ValueError("Number of atom should be an integer.")
        if num_atom < 0 or num_atom > self._num_atoms: # here we count from zero
            raise ValueError("Invalid number of atom.")

        if not isinstance(item, np.ndarray):
            raise ValueError("Debye-Waller factor should have a form of a numpy array.")
        if item.shape != (3,3):
            raise ValueError("Debye-Waller factor should have a form of 3x3 numpy array"
                             " (outer product of atomic displacements).")
        if item.dtype.num != Constants.floats_id:
            raise ValueError("Invalid type of DW factors. Floating numbers are expected.")

        self._data[num_atom,:,:] += item


    def set(self, items=None):
        """
        Sets a new value for DW factors.
        @param items: new value of DW
        """
        if not isinstance(items, np.ndarray):
            raise ValueError("New value of DW factors should be a numpy array.")
        if items.shape[0] != self._num_atoms:
            raise ValueError("Size of Debye-Waller factors data and number of atoms are inconsistent.")
        if items.shape[1] != 3 or items.shape[2] != 3:
            raise ValueError("Improper size of Debye-Waller factors.")
        if items.dtype.num != Constants.floats_id:
            raise ValueError("Invalid type of DW factors. Numpy array with floating numbers is expected.")

        self._data = items


    def extract(self):

        # dimensions of the data
        # self._data[atom,i,j]
        # atom -- index of atom
        # i, j = 1,2,3 defines 3x3 matrix which is created from outer product of atomic displacements.

        if len(self._data.shape) != 3:
            raise ValueError("Improper format of Debye-Waller data.")
        if self._data.shape[0] != self._num_atoms:
            raise ValueError("Number of atoms and size of data is inconsistent.")
        if self._data.shape[1] != 3 or self._data.shape[2]  != 3:
            raise ValueError("3x3 matrices created from outer product of atomic displacements are expected.")
        if self._data.dtype.num != Constants.floats_id:
            raise ValueError("Invalid type of DW factors. Numpy array with floating numbers is expected.")

        return self._data
