# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from numpy.testing import assert_allclose
import json

from abins import PowderCalculator
import abins.input
import abins.test_helpers


class PowderCalculatorTest(unittest.TestCase):
    # Use case: one k-point
    _c6h6 = "benzene_CalculatePowder"

    #  Use case: many k-points
    _si2 = "Si2-sc_CalculatePowder"

    #     test input
    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["CalculatePowder"])

    def test_wrong_input(self):
        full_path_filename = abins.test_helpers.find_file(filename=self._si2 + ".phonon")

        castep_reader = abins.input.CASTEPLoader(input_ab_initio_filename=full_path_filename)
        good_data = castep_reader.read_vibrational_or_phonon_data()

        # wrong filename
        self.assertRaises(ValueError, PowderCalculator, filename=1, abins_data=good_data, temperature=300.0)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = good_data.extract()["atoms_data"]
        self.assertRaises(ValueError, PowderCalculator, filename=full_path_filename, abins_data=bad_data, temperature=300.0)

        # Missing Temperature
        self.assertRaises(TypeError, PowderCalculator, filename=full_path_filename, abins_data=good_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._c6h6)
        self._good_case(name=self._si2)

    #       helper functions
    def _good_case(self, name=None):
        # calculation of powder data
        good_data = self._get_good_data(filename=name)
        good_tester = abins.PowderCalculator(
            filename=abins.test_helpers.find_file(filename=name + ".phonon"), abins_data=good_data["DFT"], temperature=300.0
        )
        calculated_data = good_tester.calculate_data().extract()

        # check if evaluated powder data  is correct
        for key in good_data["powder"]:
            for i in good_data["powder"][key]:
                try:
                    assert_allclose(good_data["powder"][key][i], calculated_data[key][i])
                except AssertionError as e:
                    raise AssertionError(f"Difference in field '{key}'") from e

        # check if loading powder data is correct
        new_tester = abins.PowderCalculator(
            filename=abins.test_helpers.find_file(name + ".phonon"), abins_data=good_data["DFT"], temperature=300.0
        )

        loaded_data = new_tester.load_formatted_data().extract()
        for key in good_data["powder"]:
            for i in good_data["powder"][key]:
                self.assertEqual(True, np.allclose(calculated_data[key][i], loaded_data[key][i]))

    def _get_good_data(self, filename=None):
        castep_reader = abins.input.CASTEPLoader(input_ab_initio_filename=abins.test_helpers.find_file(filename + ".phonon"))
        powder = self._prepare_data(filename=abins.test_helpers.find_file(filename + "_powder.txt"))

        return {"DFT": castep_reader.read_vibrational_or_phonon_data(), "powder": powder}

    @unittest.skip("Test run - don't regenerate data")
    def test_write_reference_data(self):
        self._write_data(name=self._c6h6)
        self._write_data(name=self._si2)

    @classmethod
    def _write_data(cls, name=None):
        castep_file = abins.test_helpers.find_file(name + ".phonon")
        abins_data = abins.input.CASTEPLoader(input_ab_initio_filename=castep_file).read_vibrational_or_phonon_data()

        powder = abins.PowderCalculator(
            filename=abins.test_helpers.find_file(name + ".phonon"), abins_data=abins_data, temperature=300.0
        ).calculate_data()

        powder_dict = abins.test_helpers.dict_arrays_to_lists(powder.extract())
        json_file = castep_file.replace(".phonon", "_powder.txt")
        raise Exception(json_file)
        with open(json_file, "w") as fd:
            json.dump(powder_dict, fd, indent=4, sort_keys=True)

    @staticmethod
    def _prepare_data(filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            correct_data = json.loads(
                data_file.read()
                .replace("\\n", " ")
                .replace("array", "")
                .replace("([", "[")
                .replace("])", "]")
                .replace("'", '"')
                .replace("0. ", "0.0")
            )

        for key in correct_data.keys():
            if "tensors" in key:
                for k in correct_data[key]:
                    correct_data[key][k] = np.asarray(correct_data[key][k])

        return correct_data


if __name__ == "__main__":
    unittest.main()
