from __future__ import (absolute_import, division, print_function)
import AbinsModules


class SingleCrystalData(AbinsModules.GeneralData):
    """
    Class for storing the data in case sample has a form of single crystal.
    """

    def __init__(self):

        super(SingleCrystalData, self).__init__()
        self._dw_crystal_data = None
        self._abins_data = None

    def set(self, abins_data=None, dw_crystal_data=None):

        """
        @param abins_data: object of type AbinsData with data from DFT phonon calculation.
        @param dw_crystal_data: object of type DwCrystalData with DW for the case of crystal.
        """

        if isinstance(abins_data, AbinsModules.AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        if isinstance(dw_crystal_data, AbinsModules.DWSingleCrystalData):
            self._dw_crystal_data = dw_crystal_data
        else:
            raise ValueError("Object of type DwCrystalData was expected.")

        self._data = {"abins_data": self._abins_data.extract(), "dw_crystal_data": self._dw_crystal_data.extract()}

    def extract(self):

        if (self._data["abins_data"]["k_points_data"]["atomic_displacements"].shape[1] ==
                self._data["dw_crystal_data"].shape[0]):

            return self._data
        else:
            raise ValueError("Object fo type CrystalData is inconsistent.")

    def __str__(self):
        return "Crystal data"
