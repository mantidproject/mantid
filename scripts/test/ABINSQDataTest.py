import unittest
import numpy as np
from mantid.simpleapi import *

from AbinsModules import QData


class ABINSQvectorsTEST(unittest.TestCase):
    _good_array =  np.asarray([2.,3.,4.])

    def runTest(self):
        # wrong  frequency_dependent
        with self.assertRaises(ValueError):
            testing_vec = QData(frequency_dependence="nice")

        # Q frequency dependent: wrong items to extract
        vectors_Q = QData(frequency_dependence=True)
        vectors_Q.set_k(k=2) # k should be 1
        vectors_Q._append(item=self._good_array)
        with self.assertRaises(ValueError):
             wrong_data = vectors_Q.extract()

        # Q frequency dependent: wrong  items to append
        vectors_Q = QData(frequency_dependence=True)
        vectors_Q.set_k(k=1)
        with self.assertRaises(ValueError):
             vectors_Q._append(item=[1, 2, 4]) # list instead of numpy array

        # Q frequency dependent: wrong items to set
        vectors_Q = QData(frequency_dependence=True)
        with self.assertRaises(ValueError):
            vectors_Q.set(items=[1,2,4]) # list instead of numpy array

        # Q frequency dependent: invalid k
        vectors_Q = QData(frequency_dependence=True)
        with self.assertRaises(ValueError):
            vectors_Q.set_k(-1)

        # Q frequency independent: wrong items added
        vectors_Q = QData(frequency_dependence=False)
        vectors_Q._append(np.asarray([1.0, 3.0, 2.0, 0.0])) # should be 3 elements not 4
        with self.assertRaises(ValueError):
            wrong_data = vectors_Q.extract()

        # Q frequency independent: wrong shape of data
        vectors_Q = QData(frequency_dependence=False)

        with self.assertRaises(ValueError):
            vectors_Q.set([1.0,3.0,2.0,0.0]) # should be 2D array not 1D array

        # Q frequency dependent: good items to append
        vectors_Q = QData(frequency_dependence=True)
        vectors_Q.set_k(k=1)
        vectors_Q._append(item=self._good_array)
        self.assertEqual(True, np.allclose(self._good_array, vectors_Q.extract()[0]))

        # Q frequency dependent: good items to set
        vectors_Q = QData(frequency_dependence=True)
        vectors_Q.set_k(k=1)
        vectors_Q.set([self._good_array]) # array for one k point
        self.assertEqual(True, np.allclose(self._good_array, vectors_Q.extract()))

        # Q frequency independent: good items to append
        vectors_Q = QData(frequency_dependence=False)
        vectors_Q._append(item=self._good_array)
        vectors_Q._append(item=self._good_array)
        self.assertEqual(True, np.allclose([self._good_array, self._good_array], vectors_Q.extract()))

        # Q frequency independent: good items to set as an array
        vectors_Q = QData(frequency_dependence=False)
        vectors_Q.set([self._good_array, self._good_array])
        self.assertEqual(True, np.allclose([self._good_array, self._good_array], vectors_Q.extract()))


if __name__ == '__main__':
    unittest.main()