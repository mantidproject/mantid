import unittest
import numpy as np
from mantid.simpleapi import *

from AbinsModules import QData



class ABINSQvectorsTEST(unittest.TestCase):
    _good_array =  np.asarray([[2.,3.,4.]])

    def runTest(self):

        # invalid value of overtones
        with self.assertRaises(ValueError):
            vectors_Q = QData(quantum_order_events_num="bad")


        # Good items to set
        vectors_Q = QData(quantum_order_events_num=False)
        vectors_Q.set({"order_0": self._good_array}) # array for one k point
        self.assertEqual(True, np.allclose(self._good_array, vectors_Q.extract()["order_0"]))


if __name__ == '__main__':
    unittest.main()