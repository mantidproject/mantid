import unittest
import os

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

# import Numpy to check that it's on the system
import numpy

class NumpyTest(unittest.TestCase):
    """
        Simple test to check the numpy integration
    """
    
    def setUp(self):
        pass
        
    def test_array_output(self):

        loader = LoadCanSAS1D("LOQ_CANSAS1D.xml", "test")
        
        # Get the Y array
        x = mtd["test"].dataY(0)
        
        # Check that we got an ndarray
        #self.assertEqual(x.__class__, numpy.ndarray)
        
        # Some sanity check to verify that we have the right file
        self.assertEqual(len(x), 102)
        self.assertEqual(x[20], 2895.0)
        
        # Try modifying the data
        x[0] = 10.0
        
        # Try to get it back again
        y = mtd["test"].dataY(0)
        self.assertEqual(y[0], 10.0)

if __name__ == '__main__':
    unittest.main()
