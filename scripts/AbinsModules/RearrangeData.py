import Constants
import numpy as np

# ABINS modules
from AbinsData import  AbinsData

class RearrangeData(object):
    """
    Class for rearranging data used by ABINS.
    """
    def __init__(self):
        # all fields below should be set by inheriting class
        self._num_atoms = None
        self._temperature = None


    def _convert_to_hartree(self, abins_data=None):
        """
        Converts masses, frequencies, temperature to Hartree atomic units.
        @param abins_data: dictionary with  ABINS data
        @return: dictionary with the following entries:

                         mass_hartree - mass of atoms in the system in Hartree atomic units,

                         temperature_hartree - temperature in Hartree atomic units

                         frequencies_hartree - frequencies for all atoms and all k-points in Hartree atomic units.
        """

        atoms_data = abins_data["atoms_data"]
        k_points_data = abins_data["k_points_data"]

        _results = { "mass_hartree": np.asarray([atoms_data[n]["mass"] for n in range(self._num_atoms)]) * Constants.m_2_hartree,
                     "temperature_hartree": self._temperature * Constants.k_2_hartree,
                     "frequencies_hartree": k_points_data["frequencies"] * Constants.cm1_2_hartree}

        return _results
