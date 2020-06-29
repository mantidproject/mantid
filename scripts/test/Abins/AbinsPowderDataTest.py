# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from abins import PowderData


class AbinsPowderDataTest(unittest.TestCase):
    def setUp(self):
        # hypothetical data for two atoms
        self.good_items = {
            "a_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03]],
                                           [[0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03]]])},

            "b_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03]],
                                           [[0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03],
                                            [0.01, 0.02, 0.03]]])}}

    def test_bad_num_atoms(self):
        # wrong number of atoms
        with self.assertRaises(ValueError):
            PowderData(self.good_items, num_atoms=-2)

    def test_bad_items(self):
        # wrong items: list instead of numpy array
        bad_items = {"a_tensors": [[0.002, 0.001]], "b_tensors": [[0.002, 0.001]]}
        with self.assertRaises(ValueError):
            PowderData(bad_items, num_atoms=2)

        # wrong size of items: data only for one atom ; should be for two atoms
        bad_items = {"a_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])},
                     "b_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])}}
        with self.assertRaises(ValueError):
            PowderData(bad_items, num_atoms=2)

    def test_good_case(self):
        good_powderdata = PowderData(self.good_items, num_atoms=2)

        extracted_data = good_powderdata.extract()
        for key in self.good_items:
            for k_point in self.good_items[key]:
                self.assertEqual(True, np.allclose(self.good_items[key][k_point], extracted_data[key][k_point]))


if __name__ == '__main__':
    unittest.main()
