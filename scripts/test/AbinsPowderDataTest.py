from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
from AbinsModules import PowderData, AbinsTestHelpers


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsPowderDataTest because Python is too old.")

    is_numpy_old = AbinsTestHelpers.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping AbinsPowderDataTest because numpy is too old.")

    return is_python_old or is_numpy_old


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
