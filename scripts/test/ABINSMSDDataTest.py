import unittest
from mantid.simpleapi import *
import numpy as np

from AbinsModules import MSDData

class ABINSMSDDatTest(unittest.TestCase):


    def test_input(self):

        # wrong temperature
        with self.assertRaises(ValueError):
            poor_tester = MSDData(temperature=-10, num_atoms=2)

        # wrong number of atoms
        with self.assertRaises(ValueError):
            poor_tester = MSDData(temperature=10, num_atoms=-2)


    def test_append(self):

        poor_tester = MSDData(temperature=10, num_atoms=2)

        # wrong number value of mean square displacement
        with self.assertRaises(ValueError):
            poor_tester._append(num_atom=0, msd_atom="wrong_displacement")
        # wrong number of atom
        with self.assertRaises(ValueError):
            poor_tester._append(num_atom=3, msd_atom=.0001)


    def test_set(self):

        poor_tester = MSDData(temperature=10, num_atoms=2)

        # wrong items: list instead of numpy array
        bad_items = [0.002, 0.001]
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)

        # wrong size of items
        bad_items = np.asarray([0.01, 0.02, 0.03])
        with self.assertRaises(ValueError):
            poor_tester.set(items=bad_items)





    def test_good_case(self):
        good_msd = np.asarray([0.01,0.001])
        good_tester = MSDData(num_atoms=2, temperature=2)
        good_tester.set(items=good_msd)

        self.assertEqual(True, np.allclose(good_msd, good_tester.extract()))


        good_tester._append(num_atom=0, msd_atom=0.01)
        good_tester._append(num_atom=1, msd_atom=0.001)
        self.assertEqual(True, np.allclose(good_msd, good_tester.extract()))

if __name__ == '__main__':
    unittest.main()
