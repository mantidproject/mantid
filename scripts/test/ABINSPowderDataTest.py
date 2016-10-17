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

        # wrong temperature
        with self.assertRaises(ValueError):
            poor_tester = PowderData(temperature=-10, num_atoms=2)

        # wrong number of atoms
        with self.assertRaises(ValueError):
            poor_tester = PowderData(temperature=10, num_atoms=-2)


    def test_append(self):

        poor_tester = PowderData(temperature=10, num_atoms=2)

        # wrong number value of mean square displacement
        with self.assertRaises(ValueError):
            poor_tester._append(num_atom=0, powder_atom="wrong_displacement")
        # wrong number of atom
        with self.assertRaises(ValueError):
            poor_tester._append(num_atom=3, powder_atom={"msd": 0.0001, "dw": 0.0001})


    def test_set(self):

        poor_tester = PowderData(temperature=10, num_atoms=2)

        # wrong items: list instead of numpy array
        bad_items = {"msd": [0.002, 0.001], "dw": [0.002, 0.001]}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)

        # wrong size of items
        bad_items = {"msd": np.asarray([0.01, 0.02, 0.03]), "dw": np.asarray([0.01, 0.02, 0.03])}
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)





    def test_good_case(self):

        good_msd = {"msd": np.asarray([0.01, 0.001]), "dw": np.asarray([0.01, 0.001])}
        good_tester = PowderData(temperature=2, num_atoms=2)
        good_tester.set(items=good_msd)

        extracted_data = good_tester.extract()
        for key in good_msd:
            self.assertEqual(True, np.allclose(good_msd[key], extracted_data[key]))

        good_tester._append(num_atom=0, powder_atom={"dw": 0.01,  "msd": 0.01})
        good_tester._append(num_atom=1, powder_atom={"dw": 0.001, "msd": 0.001})
        extracted_data = good_tester.extract()
        for key in good_msd:
            self.assertEqual(True, np.allclose(good_msd[key], extracted_data[key]))

if __name__ == '__main__':
    unittest.main()
