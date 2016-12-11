import unittest
from mantid.simpleapi import logger
import os
import numpy as np
import json
from AbinsModules import AbinsTestHelpers, CalculateCrystal, LoadCASTEP


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSCalculateCrystalTest because Python is too old.")
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
class ABINSCalculateCrystalTest(unittest.TestCase):

    core = AbinsTestHelpers.get_core_folder()
    _temperature = 10  # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    _C6H6 = os.path.join(core, "benzene_CalculateCrystal")

    #  Use case: many k-points
    _Si2 = os.path.join(core, "Si2-sc_CalculateCrystal")

    #     test input
    def test_wrong_input(self):

        filename = self._Si2 + ".phonon"

        castep_reader = LoadCASTEP(input_dft_filename=filename)
        good_data = castep_reader.read_phonon_file()

        # wrong filename
        self.assertRaises(ValueError, CalculateCrystal, filename=1,
                          temperature=self._temperature, abins_data=good_data)

        # wrong temperature
        self.assertRaises(ValueError, CalculateCrystal, filename=filename, temperature=-10, abins_data=good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = good_data.extract()["atoms_data"]
        self.assertRaises(ValueError, CalculateCrystal, filename=filename,
                          temperature=self._temperature, abins_data=bad_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._C6H6)
        self._good_case(name=self._Si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of crystal data
        good_data = self._get_good_data(filename=name)
        good_tester = CalculateCrystal(filename=name + ".phonon",
                                       temperature=self._temperature,
                                       abins_data=good_data["DFT"])
        calculated_data = good_tester.calculate_data().extract()

        # check if evaluated crystal data  is correct
        self.assertEqual(True, np.allclose(good_data["dw_crystal_data"], calculated_data["dw_crystal_data"]))

        # check if loading crystal data is correct
        new_tester = CalculateCrystal(filename=name + ".phonon",
                                      temperature=self._temperature,
                                      abins_data=good_data["DFT"])
        loaded_data = new_tester.load_data().extract()

        self.assertEqual(True, np.allclose(calculated_data["dw_crystal_data"], loaded_data["dw_crystal_data"]))

    def _get_good_data(self, filename=None):

        castep_reader = LoadCASTEP(input_dft_filename=filename + ".phonon")
        crystal = self._prepare_data(filename=filename + "_crystal_DW.txt")

        return {"DFT": castep_reader.read_phonon_file(), "dw_crystal_data": crystal}

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " ").
                                      replace("array", "").
                                      replace("([",    "[").
                                      replace("])",    "]").
                                      replace("'",     '"'))

        correct_data = np.asarray(correct_data)

        return correct_data

if __name__ == '__main__':
    unittest.main()
