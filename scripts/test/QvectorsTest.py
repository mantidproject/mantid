import unittest
import numpy as np
from mantid.simpleapi import *

from AbinsModules import Qvectors


class QvectorsTEST(unittest.TestCase):
    def runTest(self):

        # wrong vectors
        with self.assertRaises(ValueError):
            testing_vec = Qvectors(q_format="scalars", vectors= [1,2]) # list sent, should be numpy array

        # wrong q_format
        with self.assertRaises(ValueError):
            testing_vec = Qvectors(q_format="nice", vectors= np.array([1,2,3]))




if __name__ == '__main__':
    unittest.main()