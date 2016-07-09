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

        self._check_item(data=item, atom=num_atom)
        self._data[num_atom,:,:] = item


    def set(self, items=None):
        """
        Sets a new value for DW factors.
        @param items: new value of DW
        """

        self._check_items(data=items)
        self._data = items


    def extract(self):

        # dimensions of the data
        # self._data[atom, i, j]
        # atom -- index of atom
        # i, j = 1, 2, 3 define 3x3 matrix which is created from outer product of atomic displacements.

        self._check_items(data=self._data)
        return self._data


    def _check_item(self, data=None, atom=None):
        """
        Checks if structure of Debye-Waller factor is valid.
        @param data: Debye-Waller factor to check
        @param atom:  number of atom
        """
        if not isinstance(atom, int):
            raise ValueError("Number of atom should be an integer.")
        if atom < 0 or atom > self._num_atoms: # here we count from zero
            raise ValueError("Invalid number of atom.")

        if not isinstance(data, np.ndarray):
            raise ValueError("Debye-Waller factor should have a form of a numpy array.")
        if data.shape != (3,3):
            raise ValueError("Debye-Waller factor should have a form of 3x3 numpy array"
                             " (outer product of atomic displacements).")
        if data.dtype.num != Constants.floats_id:
            raise ValueError("Invalid type of DW factors. Floating numbers are expected.")


    def _check_items(self, data=None):
        """
        Checks if data id valid.
        @param data: data to check
        """
        if not isinstance(data, np.ndarray):
            raise ValueError("Improper format of Debye-Waller data.")
        if data.shape[0] != self._num_atoms:
            raise ValueError("Size of Debye-Waller factors data and number of atoms are inconsistent.")
        if data.shape[1] != 3 or data.shape[2] != 3:
            raise ValueError("Improper size of Debye-Waller factors.")
        if data.dtype.num != Constants.floats_id:
            raise ValueError("Invalid type of DW factors. Numpy array with floating numbers is expected.")