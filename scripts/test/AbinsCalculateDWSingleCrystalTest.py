from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import *
import numpy as np
import json

from AbinsModules import CalculateDWSingleCrystal, LoadCASTEP, AbinsTestHelpers


class AbinsCalculateDWSingleCrystalTest(unittest.TestCase):

    temperature = 10  # 10 K

    # data
    # Use case: one k-point
    _c6h6 = "benzene_CalculateDWSingleCrystal"

    #  Use case: many k-points
    _si2 = "Si2-sc_CalculateDWSingleCrystal"

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["CalculateDWSingleCrystal"])

    # simple tests
    def test_wrong_input(self):
        filename = self._si2 + ".phonon"

        castep_reader = LoadCASTEP(input_ab_initio_filename=AbinsTestHelpers.find_file(filename=filename))
        good_data = castep_reader.read_vibrational_or_phonon_data()

        # wrong temperature
        self.assertRaises(ValueError, CalculateDWSingleCrystal, temperature=-10, abins_data=good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = good_data.extract()["atoms_data"]
        self.assertRaises(ValueError, CalculateDWSingleCrystal, temperature=self.temperature, abins_data=bad_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._c6h6)
        self._good_case(name=self._si2)

    #       helper functions
    def _good_case(self, name=None):

        # calculation of DW
        good_data = self._get_good_data(filename=name)

        good_tester = CalculateDWSingleCrystal(temperature=self.temperature, abins_data=good_data["DFT"])
        calculated_data = good_tester.calculate_data()

        # check if evaluated DW are correct
        self.assertEqual(True, np.allclose(good_data["DW"], calculated_data.extract()))

    def _get_good_data(self, filename=None):

        castep_reader = LoadCASTEP(input_ab_initio_filename=AbinsTestHelpers.find_file(filename=filename + ".phonon"))
        dw = self._prepare_data(filename=AbinsTestHelpers.find_file(filename=filename + "_crystal_DW.txt"))

        return {"DFT": castep_reader.read_vibrational_or_phonon_data(), "DW": dw}

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))
        return np.asarray(correct_data)

if __name__ == '__main__':
    unittest.main()
