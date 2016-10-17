import numpy as np

from GeneralData import  GeneralData
import  AbinsConstants

class PowderData(GeneralData):
    """
    Class for storing powder data. Powder data has a from of the dictionary with the following entries:

       msd - mean square displacements in the from of numpy array

       dw  - Debye-Waller factors for mean square displacements in the form of numpy array

    """
    def __init__(self, temperature=None, num_atoms=None):
        super(PowderData, self).__init__()
        if not ((isinstance(temperature, float) or isinstance(temperature, int)) and temperature > 0):
            raise ValueError("Invalid value of temperature.")
        self._temperature = temperature

        if isinstance(num_atoms, int) and num_atoms > 0:
            self._num_atoms = num_atoms
        else:
            raise ValueError("Invalid value of atoms.")

        self._data = {"msd": np.zeros(shape=self._num_atoms, dtype=AbinsConstants.float_type),  # value of mean square displacements
                      "dw":  np.zeros(shape=self._num_atoms, dtype=AbinsConstants.float_type)} # Debye-Waller factor for that mean square displacements


    def _append(self, num_atom=None, powder_atom=None):
        if not (isinstance(num_atom, int) and 0 <= num_atom < self._num_atoms):
            raise ValueError("Invalid number of atom.")

        if not isinstance(powder_atom, dict):
            raise ValueError("Invalid value. Dictionary with the following entries : %s" % AbinsConstants.all_keywords_powder_data + " was expected")

        if sorted(powder_atom.keys()) != sorted(AbinsConstants.all_keywords_powder_data):
            raise ValueError("Invalid structure of the dictionary.")

        if isinstance(powder_atom["msd"], float):
            self._data["msd"][num_atom] = powder_atom["msd"]
        else:
            raise ValueError("Invalid value of mean square displacement (%s)." % type(powder_atom["msd"]))

        if isinstance(powder_atom["dw"], float):
            self._data["dw"][num_atom] = powder_atom["dw"]
        else:
            raise ValueError("Invalid value of mean square displacement (%s)." % type(powder_atom["dw"]))


    def set(self, items=None):

        self._check_items(items=items)
        self._data = items


    def extract(self):
        self._check_items(items=self._data)
        return self._data


    def _check_items(self, items=None):

         if not isinstance(items, dict):
             raise ValueError("Invalid value. Dictionary with the following entries : %s" % AbinsConstants.all_keywords_powder_data + " was expected")

         if sorted(items.keys()) != sorted(AbinsConstants.all_keywords_powder_data):
            raise ValueError("Invalid structure of the dictionary.")

         if not isinstance(items["msd"], np.ndarray):
            raise ValueError("New value of MSD should be a numpy array.")
         if not isinstance(items["dw"], np.ndarray):
            raise ValueError("New value of Debye-Waller factors should be a numpy array.")

         if items["msd"].shape != (self._num_atoms,):
            raise ValueError("Invalid size of mean square displacements.")
         if items["dw"].shape != (self._num_atoms, ):
            raise ValueError("Invalid size of Debye-Waller factors.")


    def __str__(self):
        return "Powder data"