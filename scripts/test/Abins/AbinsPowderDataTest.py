# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from pydantic import ValidationError
import numpy as np

from abins import PowderData


class AbinsPowderDataTest(unittest.TestCase):
    def setUp(self):
        # hypothetical data for two atoms
        self.good_items = {
            "a_tensors": {
                0: np.asarray(
                    [
                        [[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]],
                        [[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]],
                    ]
                )
            },
            "b_tensors": {
                0: np.asarray(
                    [
                        [[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]],
                        [[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]],
                    ]
                )
            },
            "frequencies": {0: np.asarray([2.34, 5.67, 8.90])},
            "n_plus_1": {0: np.asarray([1.0, 1.0, 1.0])},
        }

    def test_bad_num_atoms(self):
        # wrong number of atoms
        with self.assertRaises(ValueError):
            PowderData(**self.good_items, num_atoms=-2)

    def test_bad_items(self):
        # wrong items: array instead of dict
        bad_items = self.good_items.copy()
        bad_items["a_tensors"] = bad_items["a_tensors"][0]
        with self.assertRaisesRegex(ValidationError, "Input should be a valid dictionary"):
            PowderData(**bad_items, num_atoms=2)

        # list instead of np array
        bad_items = self.good_items.copy()
        for key, value in bad_items.items():
            bad_items[key][0] = value[0].tolist()
        with self.assertRaisesRegex(ValidationError, "Input should be an instance of ndarray"):
            PowderData(**bad_items, num_atoms=2)

        # wrong size of items: data only for one atom ; should be for two atoms
        bad_items = {
            "a_tensors": {0: np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])},
            "b_tensors": {0: np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])},
            "frequencies": {0: np.asarray([1.23, 4.56, 7.89])},
            "n_plus_1": {0: np.asarray([1.0, 1.0, 1.0])},
        }
        with self.assertRaises(ValueError):
            PowderData(**bad_items, num_atoms=2)

    def test_good_case(self):
        good_powderdata = PowderData(**self.good_items, num_atoms=2)

        extracted_data = good_powderdata.extract()
        for key in self.good_items:
            for k_index in self.good_items[key]:
                self.assertTrue(np.allclose(self.good_items[key][k_index], extracted_data[key][str(k_index)]))

        # Should also work if num_atoms is not given
        PowderData(**self.good_items)

    def test_roundtrip(self):
        initial_powderdata = PowderData(**self.good_items, num_atoms=2)
        roundtrip_data = PowderData.from_extracted(initial_powderdata.extract())
        for attr in "get_a_tensors", "get_b_tensors", "get_frequencies":
            for k_index in self.good_items["a_tensors"]:
                self.assertTrue(np.allclose(getattr(initial_powderdata, attr)()[k_index], getattr(roundtrip_data, attr)()[k_index]))

    def test_getters(self):
        good_powderdata = PowderData(**self.good_items, num_atoms=2)
        for k_point in self.good_items["a_tensors"]:
            self.assertTrue(np.allclose(self.good_items["a_tensors"][k_point], good_powderdata.get_a_tensors()[k_point]))
            self.assertTrue(np.allclose(self.good_items["b_tensors"][k_point], good_powderdata.get_b_tensors()[k_point]))
            self.assertTrue(np.allclose(self.good_items["frequencies"][k_point], good_powderdata.get_frequencies()[k_point]))


if __name__ == "__main__":
    unittest.main()
