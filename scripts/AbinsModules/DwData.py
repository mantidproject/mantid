from GeneralData import GeneralData
import  numpy as np

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


    def append(self, item=None):
        """
        Appends DW tensor for one atom.
        @param item:  DW tensor for one atom.

        """
        if not isinstance(item, np.ndarray):
            raise ValueError("Debye-Waller factor should have a form of a numpy array.")
        if item.shape != (3,3):
            raise ValueError("Debye-Waller factor should have a form of 3x3 numpy array"
                             " (outer product of atomic displacements).")

        self._data.append(item)


    def set(self, items=None):
        """
        Sets a new value for DW factors.
        @param items: new value of DW
        """
        if isinstance(items, list): # case when DW are calculated
            if len(items) != self._num_atoms:
                raise ValueError("Size of Debye-Waller factors data and number of atoms are inconsistent.")
        elif isinstance(items, np.ndarray): # case when DW factors are loaded from hdf file
            if items.shape[0] != self._num_atoms:
                raise ValueError("Size of Debye-Waller factors data and number of atoms are inconsistent.")

        super(DwData, self).set(items=items)


    def extract(self):

        if isinstance(self._data, list):
            self._data = np.asarray(self._data)

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


        return self._data
