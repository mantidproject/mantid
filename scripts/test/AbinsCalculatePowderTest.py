# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
import AbinsModules
import json


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

        castep_reader = AbinsModules.LoadCASTEP(input_ab_initio_filename=full_path_filename)
        good_data = castep_reader.read_vibrational_or_phonon_data()

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
            input_ab_initio_filename=AbinsModules.AbinsTestHelpers.find_file(filename + ".phonon"))
        powder = self._prepare_data(filename=AbinsModules.AbinsTestHelpers.find_file(filename + "_powder.txt"))

        return {"DFT": castep_reader.read_vibrational_or_phonon_data(), "powder": powder}

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
