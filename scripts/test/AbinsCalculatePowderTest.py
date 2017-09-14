from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
import AbinsModules
import json


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_numpy_old = AbinsModules.AbinsTestHelpers.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping AbinsCalculatePowderTest because numpy is too old.")

    return is_numpy_old


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
class AbinsCalculatePowderTest(unittest.TestCase):

    # data
    # Use case: one k-point
    _c6h6 = "benzene_CalculatePowder"

    #  Use case: many k-points
    _si2 = "Si2-sc_CalculatePowder"

    #     test input
    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["CalculatePowder"])

    def test_wrong_input(self):

        full_path_filename = AbinsModules.AbinsTestHelpers.find_file(filename=self._si2 + ".phonon")

        castep_reader = AbinsModules.LoadCASTEP(input_dft_filename=full_path_filename)
        good_data = castep_reader.read_phonon_file()

        # wrong filename
        self.assertRaises(ValueError, AbinsModules.CalculatePowder,
                          filename=1, abins_data=good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = good_data.extract()["atoms_data"]
        self.assertRaises(ValueError, AbinsModules.CalculatePowder,
                          filename=full_path_filename, abins_data=bad_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._c6h6)
        self._good_case(name=self._si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of powder data
        good_data = self._get_good_data(filename=name)

        good_tester = AbinsModules.CalculatePowder(
            filename=AbinsModules.AbinsTestHelpers.find_file(filename=name + ".phonon"),
            abins_data=good_data["DFT"])
        calculated_data = good_tester.calculate_data().extract()

        # check if evaluated powder data  is correct
        for key in good_data["powder"]:
            for i in good_data["powder"][key]:
                self.assertEqual(True, np.allclose(good_data["powder"][key][i], calculated_data[key][i]))

        # check if loading powder data is correct
        new_tester = AbinsModules.CalculatePowder(
            filename=AbinsModules.AbinsTestHelpers.find_file(name + ".phonon"),
            abins_data=good_data["DFT"])
        loaded_data = new_tester.load_formatted_data().extract()
        for key in good_data["powder"]:
            for i in good_data["powder"][key]:
                self.assertEqual(True, np.allclose(calculated_data[key][i], loaded_data[key][i]))

    def _get_good_data(self, filename=None):

        castep_reader = AbinsModules.LoadCASTEP(
            input_dft_filename=AbinsModules.AbinsTestHelpers.find_file(filename + ".phonon"))
        powder = self._prepare_data(filename=AbinsModules.AbinsTestHelpers.find_file(filename + "_powder.txt"))

        return {"DFT": castep_reader.read_phonon_file(), "powder": powder}

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
            for k in correct_data[key]:
                correct_data[key][k] = np.asarray(correct_data[key][k])

        return correct_data


if __name__ == '__main__':
    unittest.main()
