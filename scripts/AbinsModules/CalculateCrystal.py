from IOmodule import  IOmodule
from CalculateDWCrystal import CalculateDWCrystal
from DwCrystalData import DwCrystalData
from CrystalData import CrystalData
from AbinsData import AbinsData
import AbinsParameters

class CalculateCrystal(IOmodule):
    def __init__(self, filename=None, abins_data=None, temperature=None):
        """
        @param filename:  name of input DFT filename
        @param abins_data: object of type AbinsData with data from phonon DFT file
        @param temperature:  temperature in K
        """
        if not isinstance(abins_data, AbinsData):
            raise ValueError("Object of AbinsData was expected.")
        self._abins_data = abins_data

        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of temperature.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature) # temperature in K

        super(CalculateCrystal, self).__init__(input_filename=filename, group_name=AbinsParameters.crystal_data_group + "/" + "%sK"%self._temperature)


    def _calculate_crystal(self):

        _dw_crystal = CalculateDWCrystal(temperature=self._temperature, abins_data=self._abins_data)
        _dw_crystal_data = _dw_crystal.calculateData()
        _crystal_data = CrystalData()
        _crystal_data.set(abins_data=self._abins_data, dw_crystal_data=_dw_crystal_data)

        return _crystal_data


    def calculateData(self):
        """
        Calculates data needed for calculation of S(Q, omega) in case experimental sample is in  the form of single crystal.
        Saves calculated data to an hdf file.
        @return:  object of type CrystalData
        """

        data = self._calculate_crystal()
        self.addFileAttributes()
        self.addData("dw", data.extract()["dw_crystal_data"]) # AbinsData is already stored in an hdf file so only data for DW factors should be stored.

        self.save()

        return data


    def loadData(self):

        _data = self.load(list_of_datasets=["dw"])
        _num_atoms = _data["datasets"]["dw"].shape[0]

        _dw_crystal_data = DwCrystalData(temperature=self._temperature, num_atoms=_num_atoms)
        _dw_crystal_data.set(items=_data["datasets"]["dw"])

        _crystal_data = CrystalData()
        _crystal_data.set(abins_data=self._abins_data, dw_crystal_data=_dw_crystal_data)

        return _crystal_data
