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
from AbinsModules import PowderData, AbinsTestHelpers


class AbinsPowderDataTest(unittest.TestCase):

    def test_input(self):

        # wrong number of atoms
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = PowderData(num_atoms=-2)

    def test_set(self):

        poor_tester = PowderData(num_atoms=2)

        # wrong items: list instead of numpy array
        bad_items = {"a_tensors": [[0.002, 0.001]], "b_tensors": [[0.002, 0.001]]}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)

        # wrong size of items: data only for one atom ; should be for two atoms
        bad_items = {"a_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])},
                     "b_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])}}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)

    def test_good_case(self):

        # hypothetical data for two atoms
        good_powder = {"a_tensors": {"0": np.asarray([[[0.01, 0.02, 0.03],
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
        good_tester = PowderData(num_atoms=2)
        good_tester.set(items=good_powder)

        extracted_data = good_tester.extract()
        for key in good_powder:
            for k_point in good_powder[key]:
                self.assertEqual(True, np.allclose(good_powder[key][k_point], extracted_data[key][k_point]))

if __name__ == '__main__':
    unittest.main()
