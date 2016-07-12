
# ABINS modules
from GeneralData import  GeneralData
from KpointsData import  KpointsData
from AtomsData import  AtomsDaTa

class AbinsData(GeneralData):
    def __init__(self, k_points_data=None, atoms_data=None):

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid type of k-points data.")

        if not isinstance(atoms_data, AtomsDaTa):
            raise ValueError("Invalid type of atoms data.")

        self._kpoints_data = k_points_data
        self._atoms_data = atoms_data
        self._data = {"k_points_data":k_points_data.extract(), "atoms_data":atoms_data.extract()}


    def extract(self):
        if self._data["k_points_data"]["atomic_displacements"].shape[1] != len(self._data["atoms_data"]):
            raise ValueError("Abins data is inconsistent.")

        return self._data