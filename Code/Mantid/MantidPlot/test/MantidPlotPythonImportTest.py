""" Test script to see if Mantid starts Python correctly

    This includes ensuring the Python algorithms are there.  
"""
import unittest
import mantidplottests
from mantidplottests import *
import time
import sys
import types

class MantidPlotPythonImportTest(unittest.TestCase):

    def setUp(self):
        pass
    
    def tearDown(self):
        """Clean up by closing the created window """
        pass
  
    def test_NewAPIIsDefault(self):
        self.assertTrue('mantid' in sys.modules)
  
    def test_ConjoinFilesExistsAsAFunction(self):
        # This means the Python algs were loaded
        self.assertTrue('ConjoinFiles' in globals())
        self.assertEquals(type(ConjoinFiles),types.FunctionType)

# Run the unit tests
mantidplottests.runTests(MantidPlotPythonImportTest) 
