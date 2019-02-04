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
import json
from AbinsModules import AbinsTestHelpers, CalculateSingleCrystal, LoadCASTEP


class AbinsCalculateSingleCrystalTest(unittest.TestCase):

    _temperature = 10  # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    _c6h6 = "benzene_CalculateSingleCrystal"

    #  Use case: many k-points
    _si2 = "Si2-sc_CalculateSingleCrystal"

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["CalculateSingleCrystal"])

    #     test input
    def test_wrong_input(self):

        filename = self._si2 + ".phonon"
        full_path_filename = AbinsTestHelpers.find_file(filename=filename)

        castep_reader = LoadCASTEP(input_ab_initio_filename=full_path_filename)
        good_data = castep_reader.read_vibrational_or_phonon_data()

        # wrong filename
        self.assertRaises(ValueError, CalculateSingleCrystal, filename=1,
                          temperature=self._temperature, abins_data=good_data)

        # wrong temperature
        self.assertRaises(ValueError, CalculateSingleCrystal, filename=full_path_filename,
                          temperature=-10, abins_data=good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = good_data.extract()["atoms_data"]
        self.assertRaises(ValueError, CalculateSingleCrystal, filename=full_path_filename,
                          temperature=self._temperature, abins_data=bad_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._c6h6)
        self._good_case(name=self._si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of crystal data
        good_data = self._get_good_data(filename=name)
        good_tester = CalculateSingleCrystal(filename=AbinsTestHelpers.find_file(filename=name + ".phonon"),
                                             temperature=self._temperature,
                                             abins_data=good_data["DFT"])
        calculated_data = good_tester.calculate_data().extract()

        # check if evaluated crystal data  is correct
        self.assertEqual(True, np.allclose(good_data["dw_crystal_data"], calculated_data["dw_crystal_data"]))

        # check if loading crystal data is correct
        new_tester = CalculateSingleCrystal(filename=AbinsTestHelpers.find_file(filename=name + ".phonon"),
                                            temperature=self._temperature,
                                            abins_data=good_data["DFT"])
        loaded_data = new_tester.load_data().extract()

        self.assertEqual(True, np.allclose(calculated_data["dw_crystal_data"], loaded_data["dw_crystal_data"]))

    def _get_good_data(self, filename=None):

        castep_reader = LoadCASTEP(input_ab_initio_filename=AbinsTestHelpers.find_file(filename=filename + ".phonon"))
        crystal = self._prepare_data(filename=AbinsTestHelpers.find_file(filename=filename + "_crystal_DW.txt"))

        return {"DFT": castep_reader.read_vibrational_or_phonon_data(), "dw_crystal_data": crystal}

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
