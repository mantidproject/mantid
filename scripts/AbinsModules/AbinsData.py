
# ABINS modules
from GeneralData import  GeneralData
from KpointsData import  KpointsData
from AtomsData import  AtomsDaTa

class AbinsData(GeneralData):
    """
    Class for storing input DFT data.
    """
    def __init__(self, ):

        super(AbinsData, self).__init__()
        self._atoms_data = None
        self._kpoints_data = None


    def set(self, k_points_data=None, atoms_data=None):
        """

        @param k_points_data: object of type KpointsData
        @param atoms_data: object of type AtomsData
        """

        if isinstance(k_points_data, KpointsData):
            self._kpoints_data = k_points_data
        else:
            raise ValueError("Invalid type of k-points data.")

        if isinstance(atoms_data, AtomsDaTa):
            self._atoms_data = atoms_data
        else:
            raise ValueError("Invalid type of atoms data.")

        self._data = {"k_points_data":k_points_data.extract(), "atoms_data":atoms_data.extract()}


    def getKpointsData(self):
        return self._kpoints_data


    def getAtomsData(self):
        return self._atoms_data


    def extract(self):
        if self._data["k_points_data"]["atomic_displacements"].shape[1] != len(self._data["atoms_data"]):
            raise ValueError("Abins data is inconsistent.")

        return self._data


    def __str__(self):
        return "DFT data"