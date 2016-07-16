import unittest
from mantid.simpleapi import *
from os import path
import simplejson as json
import numpy as np

# ABINS modules
from AbinsModules import CalculateDWCrystal
from AbinsModules import LoadCASTEP


class ABINSCalculateDWCrystalTest(unittest.TestCase):

    _core = "../ExternalData/Testing/Data/UnitTest/" # path to files
    _temperature = 10 # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = _core + "benzene"

    #  Use case: many k-points
    Si2 = _core + "Si2-sc"


    #      simple tests
    def test_wrong_input(self):
        filename = self.Si2 + ".phonon"

        _castep_reader =  LoadCASTEP(input_DFT_filename=filename)

        _good_data = _castep_reader.readPhononFile()
        # wrong filename
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDWCrystal(filename=1, temperature=self._temperature, abins_data=_good_data)

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDWCrystal(filename=filename, temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDWCrystal(filename=filename, temperature=self._temperature, abins_data=bad_data)


    #       main test
    def test_good_case(self):
        self._good_case(name=self.C6H6)
        self._good_case(name=self.Si2)



    #       helper functions
    def _good_case(self, name=None):


        # calculation of DW
        _good_data = self._get_good_data(filename=name)

        _good_tester = CalculateDWCrystal(filename=name + ".phonon", temperature=self._temperature, abins_data=_good_data["DFT"])
        calculated_data = _good_tester.getDW()

        # check if evaluated DW are correct
        self.assertEqual(True, np.allclose(_good_data["DW"], calculated_data.extract()))

        # check loading DW
        new_tester =  CalculateDWCrystal(filename=name + ".phonon", temperature=self._temperature, abins_data=_good_data["DFT"])
        loaded_data = new_tester.loadData()
        self.assertEqual(True, np.allclose(calculated_data.extract(), loaded_data.extract()))


    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _dw = self._prepare_data(filename=filename + "_DW.txt")

        return {"DFT":_CASTEP_reader.readPhononFile(), "DW": _dw}


    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n"," "))
        return np.asarray(correct_data)


if __name__ == '__main__':
    unittest.main()