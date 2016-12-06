import unittest
from mantid.simpleapi import *
import numpy as np

try:
    import json
except ImportError:
    logger.warning("Failure of DwCrystalDataTest  because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of DwCrystalDataTest because h5py is unavailable.")
    exit(1)

from AbinsModules import DwCrystalData


class ABINSDwCrystalDataTest(unittest.TestCase):
    # fake DW tensors for two atoms
    _good_data = np.asarray([[[1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0]],     # array 3x3
                             [[1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0],
                              [1.0, 1.0, 1.0]]])  # array 3x3

    def setUp(self):
        self.tester = DwCrystalData(temperature=20, num_atoms=2)

    def test_bad_input(self):

        # invalid temperature
        with self.assertRaises(ValueError):
            poor_tester = DwCrystalData(temperature=-1, num_atoms=2)

        # invalid number of atoms
        with self.assertRaises(ValueError):
            poor_tester = DwCrystalData(temperature=10, num_atoms=-2)

    def test_wrong_append(self):
        # list instead of numpy array
        _bad_item = [[1.0, 1.0, 1.0],
                     [1.0, 1.0, 1.0],
                     [1.0, 1.0, 1.0]]  # list 3x3

        with self.assertRaises(ValueError):
            self.tester._append(item=_bad_item, num_atom=0)

        # bad shape of numpy array
        _bad_item = np.asarray([[1.0, 1.0, 1.0],
                                [1.0, 1.0, 1.0]])  # array 2x3 instead of 3x3

        with self.assertRaises(ValueError):
            self.tester._append(item=_bad_item, num_atom=0)

        # bad type of elements: integers instead of floats
        _bad_item = np.asarray([[1, 1, 1],
                                [1, 1, 1],
                                [1, 1, 1]])  # array 3x3
        with self.assertRaises(ValueError):
            self.tester._append(item=_bad_item, num_atom=0)

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
