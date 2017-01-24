from __future__ import (absolute_import, division, print_function)
import AbinsModules


class CalculateSingleCrystal(AbinsModules.IOmodule):
    def __init__(self, filename=None, abins_data=None, temperature=None):
        """
        @param filename:  name of input DFT filename
        @param abins_data: object of type AbinsData with data from phonon DFT file
        @param temperature:  temperature in K
        """
        if not isinstance(abins_data, AbinsModules.AbinsData):
            raise ValueError("Object of AbinsData was expected.")
        self._abins_data = abins_data

        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of temperature.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)  # temperature in K

        super(CalculateSingleCrystal, self).__init__(
            input_filename=filename,
            group_name=AbinsModules.AbinsParameters.crystal_data_group + "/" + "%sK" % self._temperature)

    def _calculate_crystal(self):

        _dw_crystal = AbinsModules.CalculateDWSingleCrystal(temperature=self._temperature, abins_data=self._abins_data)
        _dw_crystal_data = _dw_crystal.calculate_data()
        _crystal_data = AbinsModules.SingleCrystalData()
        _crystal_data.set(abins_data=self._abins_data, dw_crystal_data=_dw_crystal_data)

        return _crystal_data

    def calculate_data(self):
        """
        Calculates data needed for calculation of S(Q, omega) in case experimental sample is in
        the form of single crystal.
        Saves calculated data to an hdf file.
        @return:  object of type SingleCrystalData
        """

        data = self._calculate_crystal()
        self.add_file_attributes()

        # AbinsData is already stored in an hdf file so only data for DW factors should be stored.
        self.add_data("dw", data.extract()["dw_crystal_data"])

        self.save()

        return data

    def load_data(self):

        _data = self.load(list_of_datasets=["dw"])
        _num_atoms = _data["datasets"]["dw"].shape[0]

        _dw_crystal_data = AbinsModules.DWSingleCrystalData(temperature=self._temperature, num_atoms=_num_atoms)
        _dw_crystal_data.set(items=_data["datasets"]["dw"])

        _crystal_data = AbinsModules.SingleCrystalData()
        _crystal_data.set(abins_data=self._abins_data, dw_crystal_data=_dw_crystal_data)

        return _crystal_data
