# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
from pathlib import Path
import unittest

import numpy as np
from numpy.testing import assert_allclose

from abins import AbinsData, PowderCalculator
import abins.input
import abins.test_helpers


class PowderCalculatorTest(unittest.TestCase):
    # Use case: one k-point
    _c6h6 = "benzene_CalculatePowder"

    #  Use case: many k-points
    _si2 = "Si2-sc_CalculatePowder"

    #     test input
    def setUp(self):
        from mantid.kernel import ConfigService

        self._cache_directory = Path(ConfigService.getString("defaultsave.directory"))

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["CalculatePowder"], directory=self._cache_directory)

    def test_wrong_input(self):
        full_path_filename = abins.test_helpers.find_file(filename=self._si2 + ".json")
        with open(full_path_filename, "r") as fp:
            abins_data = AbinsData.from_dict(json.load(fp))

        # wrong filename
        self.assertRaises(ValueError, PowderCalculator, filename=1, abins_data=abins_data, temperature=300.0)

        # data from object of type AtomsData instead of object of type AbinsData
        wrong_type_data = abins_data.get_atoms_data()
        self.assertRaises(ValueError, PowderCalculator, filename=full_path_filename, abins_data=wrong_type_data, temperature=300.0)

        # Missing Temperature
        self.assertRaises(TypeError, PowderCalculator, filename=full_path_filename, abins_data=abins_data)

    #       main test
    def test_good_case(self):
        self._good_case(name=self._c6h6)
        self._good_case(name=self._si2)

    #       helper functions
    def _good_case(self, name=None):
        # calculation of powder data
        abins_data_filename = abins.test_helpers.find_file(filename=name + ".json")
        ref_data = self._get_ref_data(filename=abins_data_filename)

        good_tester = abins.PowderCalculator(
            filename=abins_data_filename, abins_data=ref_data["DFT"], temperature=300.0, cache_directory=self._cache_directory
        )
        calculated_data = good_tester.calculate_data().extract()

        # check if evaluated powder data  is correct
        for key in ref_data["powder"]:
            for i in ref_data["powder"][key]:
                try:
                    assert_allclose(ref_data["powder"][key][i], calculated_data[key][i], atol=1e-12)
                except AssertionError as e:
                    raise AssertionError(f"Difference in field '{key}' for case {name}") from e

        # check if loading powder data is correct
        new_tester = abins.PowderCalculator(
            filename=abins_data_filename, abins_data=ref_data["DFT"], temperature=300.0, cache_directory=self._cache_directory
        )

        loaded_data = new_tester.load_formatted_data().extract()

        for key in ref_data["powder"]:
            for i in ref_data["powder"][key]:
                assert_allclose(calculated_data[key][i], loaded_data[key][i])

    def _get_ref_data(self, filename=None):
        with open(filename, "r") as fp:
            abins_data = AbinsData.from_dict(json.load(fp))

        powder = self._prepare_data(filename=filename.replace(".json", "_powder.txt"))

        return {"DFT": abins_data, "powder": powder}

    @unittest.skip("Test run - don't regenerate data")
    def test_write_reference_data(self):
        self._write_data(name=self._c6h6)
        self._write_data(name=self._si2)

    def _write_data(self, name=None):
        abins_data_filename = abins.test_helpers.find_file(name + ".json")
        with open(abins_data_filename, "r") as fp:
            abins_data = AbinsData.from_dict(json.load(fp))

        powder = abins.PowderCalculator(
            filename=abins_data_filename, abins_data=abins_data, temperature=300.0, cache_directory=self._cache_directory
        ).calculate_data()

        powder_dict = abins.test_helpers.dict_arrays_to_lists(powder.extract())
        json_file = abins_data_filename.replace(".json", "_powder.txt")

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
