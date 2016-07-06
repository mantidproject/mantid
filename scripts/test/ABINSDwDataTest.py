import unittest
from mantid.simpleapi import *
from AbinsModules import DwData
import numpy as np

class DwDataTest(unittest.TestCase):
    # fake DW tensors for two atoms
    _good_data = [ np.asarray([[1.0, 1.0, 1.0],
                               [1.0, 1.0, 1.0],
                               [1.0, 1.0, 1.0]] # array 3x3
                             ),
                   np.asarray([[1.0, 1.0, 1.0],
                               [1.0, 1.0, 1.0],
                               [1.0, 1.0, 1.0]])] # array 3x3


    def setUp(self):
         self.tester = DwData(temperature=20, num_atoms=2)


    def test_bad_input(self):

        # invalid temperature
        with self.assertRaises(ValueError):
            poor_tester = DwData(temperature=-1, num_atoms=2)

        # invalid number of atoms
        with self.assertRaises(ValueError):
            poor_tester = DwData(temperature=10, num_atoms=-2)


    def test_wrong_append(self):
        # list instead of numpy array
        _bad_item = [[1.0, 1.0, 1.0],
                     [1.0, 1.0, 1.0],
                     [1.0, 1.0, 1.0]] # list 3x3

        with self.assertRaises(ValueError):
            self.tester.append(_bad_item)

        # bad shape of numpy array
        _bad_item = np.asarray([[1.0, 1.0, 1.0],
                                [1.0, 1.0, 1.0]]) # array 2x3 instead of 3x3

        with self.assertRaises(ValueError):
            self.tester.append(_bad_item)


    def test_wrong_set(self):

        bad_numpy_items = [self._good_data[0]]  # only one entry in the list since the number of atoms is 2 there should be 2 entries
        with self.assertRaises(ValueError):
            self.tester.set(bad_numpy_items)

        bad_list_items = [ [[1.0, 1.0, 1.0], # list 3x3 instead of numpy array
                           [1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0]],

                           [[1.0, 1.0, 1.0], # list 3x3 instead of numpy array
                           [1.0, 1.0, 1.0],
                           [1.0, 1.0, 1.0]]
                        ]

        with self.assertRaises(ValueError):
            self.tester.set(bad_list_items)


    def test_good_case(self):

        self.tester.append(self._good_data[0])
        self.tester.append(self._good_data[1])
        self.assertEqual(True, np.allclose(self._good_data, self.tester.extract()))

        self.tester.set(self._good_data)
        self.assertEqual(True, np.allclose(self._good_data, self.tester.extract()))


if __name__ == '__main__':
    unittest.main()


