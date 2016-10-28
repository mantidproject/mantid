import unittest
from mantid.simpleapi import *
from os import path
import numpy as np

try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculatePowderTest because simplejson is unavailable.")
    exit(1)


try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculatePowderTest because h5py is unavailable.")
    exit(1)


from AbinsModules import CalculatePowder, LoadCASTEP


class ABINSCalculatePowderTest(unittest.TestCase):

    _core = "../ExternalData/Testing/Data/UnitTest/"  # path to files
    _temperature = 10  # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = path.relpath(_core + "benzene_CalculatePowder")

    #  Use case: many k-points
    Si2 = path.relpath(_core + "Si2-sc_CalculatePowder")

    #     test input
    def test_wrong_input(self):

        filename = self.Si2 + ".phonon"

        _castep_reader = LoadCASTEP(input_DFT_filename=filename)
        _good_data = _castep_reader.readPhononFile()

        # wrong filename
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=1, abins_data=_good_data, temperature=self._temperature)

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=filename, abins_data=_good_data, temperature=-10)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=filename, abins_data=bad_data, temperature=self._temperature)

    #       main test
    def test_good_case(self):
        self._good_case(name=self.C6H6)
        #self._good_case(name=self.Si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of powder data
        _good_data = self._get_good_data(filename=name)

        _good_tester = CalculatePowder(filename=name + ".phonon", abins_data=_good_data["DFT"],
                                        temperature=self._temperature)
        calculated_data = _good_tester.calculateData().extract()

        # check if evaluated powder data  is correct
        for key in _good_data["powder"]:

             self.assertEqual(True, np.allclose(_good_data["powder"][key], calculated_data[key]))

        # check if loading powder data is correct
        new_tester = CalculatePowder(filename=name + ".phonon", abins_data=_good_data["DFT"],
                                      temperature=self._temperature)
        loaded_data = new_tester.loadData().extract()
        for key in _good_data["powder"]:
            self.assertEqual(True, np.allclose(calculated_data[key], loaded_data[key]))

    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _powder = self._prepare_data(filename=filename + "_powder.txt")

        return {"DFT":_CASTEP_reader.readPhononFile(), "powder": _powder}

    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\\n"    , " ").
                                                       replace("array" , "").
                                                       replace("(["    , "[").
                                                       replace("])"    , "]").
                                                       replace("'"     , '"'))

        for key in correct_data.keys():
            correct_data[key] = np.asarray(correct_data[key])

        return correct_data


if __name__ == '__main__':
    unittest.main()