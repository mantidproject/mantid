import unittest
from mantid.simpleapi import *
import numpy as np


try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of PowderDataTest because simplejson is unavailable.")
    exit(1)
try:
    import scipy
except ImportError:
    logger.warning("Failure of PowderDataTest because scipy is unavailable.")
    exit(1)
try:
    import h5py
except ImportError:
    logger.warning("Failure of PowderDataTest because h5py is unavailable.")
    exit(1)

from AbinsModules import PowderData

class ABINSPowderDataTest(unittest.TestCase):


    def test_input(self):

        # wrong number of atoms
        with self.assertRaises(ValueError):
            poor_tester = PowderData(num_atoms=-2)


    def test_set(self):

        poor_tester = PowderData(num_atoms=2)

        # wrong items: list instead of numpy array
        bad_items = {"a_tensors": [0.002, 0.001], "b_tensors": [0.002, 0.001]}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)

        # wrong size of items: data only for one atom ; should be for two atoms
        bad_items = {"a_tensors": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]]),
                     "b_tensors": np.asarray([[[0.01, 0.02, 0.03], [0.01, 0.02, 0.03], [0.01, 0.02, 0.03]]])}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)





    def test_good_case(self):

        # hypothetical data for two atoms
        good_powder = {"a_tensors": np.asarray([[[0.01, 0.02, 0.03],
                                                [0.01, 0.02, 0.03],
                                               [0.01, 0.02, 0.03]],

                                              [[0.01, 0.02, 0.03],
                                               [0.01, 0.02, 0.03],
                                              [0.01, 0.02, 0.03]]]),

                       "b_tensors": np.asarray([[[0.01, 0.02, 0.03],
                                                 [0.01, 0.02, 0.03],
                                                 [0.01, 0.02, 0.03]],

                                                [[0.01, 0.02, 0.03],
                                                 [0.01, 0.02, 0.03],
                                                 [0.01, 0.02, 0.03]]])}
        good_tester = PowderData(num_atoms=2)
        good_tester.set(items=good_powder)

        extracted_data = good_tester.extract()
        for key in good_powder:
            self.assertEqual(True, np.allclose(good_powder[key], extracted_data[key]))

if __name__ == '__main__':
    unittest.main()
