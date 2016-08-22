import unittest
from mantid.simpleapi import *
from os import path
import numpy as np

try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculateCrystalTest because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateCrystalTest because h5py is unavailable.")
    exit(1)


from AbinsModules import CalculateCrystal, LoadCASTEP

class ABINSCalculateCrystalTest(unittest.TestCase):

    _core = "../ExternalData/Testing/Data/UnitTest/" # path to files
    _temperature = 10 # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = path.relpath(_core + "benzene")

    #  Use case: many k-points
    Si2 = path.relpath(_core + "Si2-sc")

    #     test input
    def test_wrong_input(self):

        filename = self.Si2 + ".phonon"

        _castep_reader =  LoadCASTEP(input_DFT_filename=filename)
        _good_data = _castep_reader.readPhononFile()

        # wrong filename
        with self.assertRaises(ValueError):
            _poor_tester = CalculateCrystal(filename=1, temperature=self._temperature, abins_data=_good_data)

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculateCrystal(filename=filename, temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculateCrystal(filename=filename, temperature=self._temperature, abins_data=bad_data)



    #       main test
    def test_good_case(self):
        self._good_case(name=self.C6H6)
        self._good_case(name=self.Si2)


    #       helper functions
    def _good_case(self, name=None):


        # calculation of crystal data
        _good_data = self._get_good_data(filename=name)

        _good_tester = CalculateCrystal(filename=name + ".phonon", temperature=self._temperature, abins_data=_good_data["DFT"])
        calculated_data = _good_tester.calculateData().extract()

        # check if evaluated crystal data  is correct
        self.assertEqual(True, np.allclose(_good_data["dw_crystal_data"], calculated_data['dw_crystal_data']))

        # check if loading crystal data is correct
        new_tester =  CalculateCrystal(filename=name + ".phonon", temperature=self._temperature, abins_data=_good_data["DFT"])
        loaded_data = new_tester.loadData().extract()

        self.assertEqual(True, np.allclose(calculated_data["dw_crystal_data"], loaded_data["dw_crystal_data"]))


    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _crystal = self._prepare_data(filename=filename + "_crystal_DW.txt")

        return {"DFT":_CASTEP_reader.readPhononFile(), "dw_crystal_data": _crystal}


    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n"    , " ").
                                      replace("array" , "").
                                      replace("(["    , "[").
                                      replace("])"    , "]").
                                      replace("'"     , '"'))

        correct_data = np.asarray(correct_data)

        return correct_data


if __name__ == '__main__':
    unittest.main()