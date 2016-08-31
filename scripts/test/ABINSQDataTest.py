import unittest
import numpy as np
from mantid.simpleapi import *

from AbinsModules import QData



class ABINSQvectorsTEST(unittest.TestCase):
    _good_array =  np.asarray([[2.,3.,4.]])

    def runTest(self):
        # Wrong items to extract -- wrong first dimension
        vectors_Q = QData(num_k=2)
        vectors_Q._append(item=self._good_array)
        with self.assertRaises(ValueError):
             wrong_data = vectors_Q.extract()

        # Invalid k
        with self.assertRaises(ValueError):
            vectors_Q = QData(num_k=-1)

        # Wrong shape of data
        vectors_Q = QData(num_k=1)

        with self.assertRaises(ValueError):
            vectors_Q.set(np.asarray([1.0,3.0,2.0,0.0])) # should be 2D array not 1D array

        # Wrong data to set
        vectors_Q = QData(num_k=1)
        with self.assertRaises(ValueError):
            vectors_Q.set([[1.0,3.0,2.0,0.0]]) # list should be numpy array

        # Good items to set
        vectors_Q = QData(num_k=1)
        vectors_Q.set(self._good_array) # array for one k point
        self.assertEqual(True, np.allclose(self._good_array, vectors_Q.extract()))


if __name__ == '__main__':
    unittest.main()