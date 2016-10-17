import unittest
from mantid.simpleapi import *
from os import path
import numpy as np


try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculateDWCrystalTest because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateDWCrystalTest because h5py is unavailable.")
    exit(1)

from AbinsModules import CalculateDWCrystal, LoadCASTEP


class ABINSCalculateDWCrystalTest(unittest.TestCase):

    _core = "../ExternalData/Testing/Data/UnitTest/" # path to files
    _temperature = 10 # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = path.relpath(_core + "benzene_CalculateDWCrystal")

    #  Use case: many k-points
    Si2 = path.relpath(_core + "Si2-sc_CalculateDWCrystal")


    #      simple tests
    def test_wrong_input(self):
        filename = self.Si2 + ".phonon"

        _castep_reader = LoadCASTEP(input_DFT_filename=filename)

        _good_data = _castep_reader.readPhononFile()

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDWCrystal(temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDWCrystal(temperature=self._temperature, abins_data=bad_data)


    #       main test
    def test_good_case(self):
        self._good_case(name=self.C6H6)
        self._good_case(name=self.Si2)


    #       helper functions
    def _good_case(self, name=None):


        # calculation of DW
        _good_data = self._get_good_data(filename=name)

        _good_tester = CalculateDWCrystal(temperature=self._temperature, abins_data=_good_data["DFT"])
        calculated_data = _good_tester.calculateData()

        # check if evaluated DW are correct
        self.assertEqual(True, np.allclose(_good_data["DW"], calculated_data.extract()))


    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _dw = self._prepare_data(filename=filename + "_crystal_DW.txt")

        return {"DFT":_CASTEP_reader.readPhononFile(), "DW": _dw}


    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n"," "))
        return np.asarray(correct_data)


if __name__ == '__main__':
    unittest.main()