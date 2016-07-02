import unittest
import numpy as np
from mantid.simpleapi import *

from AbinsModules import QData


class ABINSQvectorsTEST(unittest.TestCase):
    def runTest(self):

        # wrong q_format
        with self.assertRaises(ValueError):
            testing_vec = QData(q_format="nice")

        # wrong items to set for scalars scenario: list instead of numpy array
        with self.assertRaises(ValueError):
            poor_Q =  QData(q_format="scalars")
            poor_Q.set(items=[1, 3, 4]) # numpy array is expected not a list

        # wrong items to set: one dimensional array is expected
        with self.assertRaises(ValueError):
            poor_Q = QData(q_format="scalars")
            poor_Q.set(items=np.asarray([[1,2],[2,3]])) # this should be 1D array

        # wrong items to set for vectors scenario: list instead of numpy array
        with self.assertRaises(ValueError):
            poor_Q =QData(q_format="vectors")
            array = [[2,3,4], [3,4,5]]
            vectors_Q = poor_Q.set(items=array)


        # wrong items to set: two dimensional array of floats is expected
        with self.assertRaises(ValueError):
            poor_Q = QData(q_format="vectors")
            poor_Q.set(items=np.asarray([[1.0,3.0]])) # this should a 2D array

        # wrong items to set: two dimensional array of floats is expected
        with self.assertRaises(ValueError):
            poor_Q = QData(q_format="vectors")
            array = np.asarray([[2,3,4], [3,4,5]])
            vectors_Q = poor_Q.set(items=array)


        # good items to set: Q set of vectors
        array = np.asarray([[2.,3.,4.], [3.,4.,5.]])
        vectors_Q = QData(q_format="vectors")
        vectors_Q.set(items=array)
        self.assertEqual(True, np.allclose(array, vectors_Q.extract()))

        # good itrems to set Q set of scalars
        array = np.asarray([2.,3.,4.])
        vectors_Q = QData(q_format="scalars")
        vectors_Q.set(items=array)
        self.assertEqual(True, np.allclose(array, vectors_Q.extract()))



if __name__ == '__main__':
    unittest.main()