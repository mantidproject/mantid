from GeneralData import GeneralData
from DwCrystalData import DwCrystalData
from KpointsData import KpointsData

class CrystalData(GeneralData):
    """
    Class for storing the data in case sample has a form of single crystal.
    """

    def __init__(self):

        super(CrystalData, self).__init__()
        self._dw_crystal_data = None
        self._k_points_data = None


    def set(self, k_points_data=None, dw_crystal_data=None):

        """
        @param k_points_data: object of type kPointsData with data from DFT phonon calculation.
        @param dw_crystal_data: object of type DwCrystalData with DW for the case of crystal.
        """

        if isinstance(k_points_data, KpointsData):
            self._k_points_data = k_points_data
        else:
            raise ValueError("Object of type KpointsData was expected.")

        if isinstance(dw_crystal_data, DwCrystalData):
            self._dw_crystal_data = dw_crystal_data
        else:
            raise ValueError("Object of type DwCrystalData was expected.")

        self._data = {"k_points_data": self._k_points_data.extract(), "dw_crystal_data": self._dw_crystal_data.extract()}


    def extract(self):

        if self._data["k_points_data"]["atomic_displacements"].shape[1] == self.data["dw_crystal_data"].shape[0]:
            return self._data
        else:
            raise ValueError("Object fo type CrystalData is inconsistent.")
