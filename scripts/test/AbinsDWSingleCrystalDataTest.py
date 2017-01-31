from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
from AbinsModules import DWSingleCrystalData, AbinsTestHelpers


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsDWSingleCrystalTest because Python is too old.")
    return is_python_old


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


@skip_if(old_python)
class AbinsDWSingleCrystalTest(unittest.TestCase):
    # fake DW tensors for two atoms
    _good_data = np.asarray([[[1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0]],     # array 3x3
                             [[1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0]]])  # array 3x3

    def setUp(self):
        self.tester = DWSingleCrystalData(temperature=20, num_atoms=2)

    def test_bad_input(self):

        # invalid temperature
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = DWSingleCrystalData(temperature=-1, num_atoms=2)

        # invalid number of atoms
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = DWSingleCrystalData(temperature=10, num_atoms=-2)

    def test_wrong_append(self):
        # list instead of numpy array
        bad_item = [[1.0, 1.0, 1.0],
                    [1.0, 1.0, 1.0],
                    [1.0, 1.0, 1.0]]  # list 3x3

        with self.assertRaises(ValueError):
            self.tester._append(item=bad_item, num_atom=0)

        # bad shape of numpy array
        bad_item = np.asarray([[1.0, 1.0, 1.0],
                               [1.0, 1.0, 1.0]])  # array 2x3 instead of 3x3

        with self.assertRaises(ValueError):
            self.tester._append(item=bad_item, num_atom=0)

        # bad type of elements: integers instead of floats
        bad_item = np.asarray([[1, 1, 1],
                               [1, 1, 1],
                               [1, 1, 1]])  # array 3x3
        with self.assertRaises(ValueError):
            self.tester._append(item=bad_item, num_atom=0)

    def test_wrong_set(self):

        bad_numpy_items = self._good_data[0]  # only one entry; there should be 2 entries
        with self.assertRaises(ValueError):
            self.tester.set(bad_numpy_items)

        # list instead of numpy array
        bad_list_items = [[[1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0]],

                          [[1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0]]
                          ]

        with self.assertRaises(ValueError):
            self.tester.set(bad_list_items)

    def test_good_case(self):

        self.tester._append(item=self._good_data[0], num_atom=0)
        self.tester._append(item=self._good_data[1], num_atom=1)
        self.assertEqual(True, np.allclose(self._good_data, self.tester.extract()))

        self.tester.set(items=self._good_data)
        self.assertEqual(True, np.allclose(self._good_data, self.tester.extract()))


if __name__ == '__main__':
    unittest.main()
