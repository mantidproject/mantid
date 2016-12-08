import unittest
from mantid.simpleapi import *
import os
import numpy as np
import json

from AbinsModules import CalculateDWCrystal, LoadCASTEP, AbinsConstants


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSCalculateDWCrystalTest because Python is too old.")
    return is_python_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_python)
class ABINSCalculateDWCrystalTest(unittest.TestCase):

    _core = os.path.normpath("../ExternalData/Testing/Data/UnitTest/")  # path to files
    _temperature = 10  # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = os.path.abspath(os.path.join(_core, "benzene_CalculateDWCrystal"))

    #  Use case: many k-points
    Si2 = os.path.abspath(os.path.join(_core, "Si2-sc_CalculateDWCrystal"))

    # simple tests
    def test_wrong_input(self):
        filename = self.Si2 + ".phonon"

        _castep_reader = LoadCASTEP(input_dft_filename=filename)

        _good_data = _castep_reader.read_phonon_file()

        # wrong temperature
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            _poor_tester = CalculateDWCrystal(temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
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
        calculated_data = _good_tester.calculate_data()

        # check if evaluated DW are correct
        self.assertEqual(True, np.allclose(_good_data["DW"], calculated_data.extract()))

    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_dft_filename=filename + ".phonon")
        _dw = self._prepare_data(filename=filename + "_crystal_DW.txt")

        return {"DFT": _CASTEP_reader.read_phonon_file(), "DW": _dw}

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))
        return np.asarray(correct_data)

if __name__ == '__main__':
    unittest.main()
