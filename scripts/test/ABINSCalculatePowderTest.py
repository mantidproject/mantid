import unittest
from mantid.simpleapi import logger
import os
import numpy as np
from AbinsModules import CalculatePowder, LoadCASTEP, AbinsConstants
import json


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSCalculatePowderTest because Python is too old.")

    is_numpy_old = AbinsConstants.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping ABINSCalculatePowderTest because numpy is too old.")

    return is_python_old or is_numpy_old


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


@skip_if(old_modules)
class ABINSCalculatePowderTest(unittest.TestCase):

    _core = os.path.normpath("../ExternalData/Testing/Data/UnitTest/")  # path to files
    _temperature = 10  # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = os.path.abspath(os.path.join(_core, "benzene_CalculatePowder"))

    #  Use case: many k-points
    Si2 = os.path.abspath(os.path.join(_core, "Si2-sc_CalculatePowder"))

    #     test input
    def test_wrong_input(self):

        filename = self.Si2 + ".phonon"

        _castep_reader = LoadCASTEP(input_dft_filename=filename)
        _good_data = _castep_reader.read_phonon_file()

        # wrong filename
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            _poor_tester = CalculatePowder(filename=1, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            _poor_tester = CalculatePowder(filename=filename, abins_data=bad_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self.C6H6)
        self._good_case(name=self.Si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of powder data
        _good_data = self._get_good_data(filename=name)

        _good_tester = CalculatePowder(filename=name + ".phonon", abins_data=_good_data["DFT"])
        calculated_data = _good_tester.calculate_data().extract()

        # check if evaluated powder data  is correct
        for key in _good_data["powder"]:

            self.assertEqual(True, np.allclose(_good_data["powder"][key], calculated_data[key]))

        # check if loading powder data is correct
        new_tester = CalculatePowder(filename=name + ".phonon", abins_data=_good_data["DFT"])
        loaded_data = new_tester.load_data().extract()
        for key in _good_data["powder"]:
            self.assertEqual(True, np.allclose(calculated_data[key], loaded_data[key]))

    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_dft_filename=filename + ".phonon")
        _powder = self._prepare_data(filename=filename + "_powder.txt")

        return {"DFT": _CASTEP_reader.read_phonon_file(), "powder": _powder}

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\\n",    " ").
                                      replace("array",  "").
                                      replace("([",     "[").
                                      replace("])",     "]").
                                      replace("'",      '"').
                                      replace("0. ",    "0.0"))

        for key in correct_data.keys():
            correct_data[key] = np.asarray(correct_data[key])

        return correct_data


if __name__ == '__main__':
    unittest.main()
