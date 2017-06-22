"""
    Test of FunctiomWrapper and related classes
"""
from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers
import platform
from mantid.simpleapi import CreateWorkspace, Fit, FitDialog, FunctionWrapper, CompositeFunctionWrapper
from mantid.api import mtd, MatrixWorkspace, ITableWorkspace
import numpy as np
from testhelpers import run_algorithm

class FunctionWrapperTest(unittest.TestCase):

    _raw_ws = None

    def setUp(self):
        pass
        
    def test_creation(self):
        testhelpers.assertRaisesNothing(self, FunctionWrapper, "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)

    def test_read_array_elements(self):
        g = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)  
        self.assertAlmostEqual(g["Height"],7.5,10)
        self.assertAlmostEqual(g[2],1.2,10)
        
    def test_write_array_elements(self):
        g = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10) 
        g["Height"] = 8
        self.assertAlmostEqual(g["Height"],8,10)
        g[2] = 1.5
        self.assertAlmostEqual(g[2],1.5,10)
        
    def test_compositefunction_creation(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, CompositeFunctionWrapper, g0, g1)  

    def test_compositefunction_read_array_elements(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper( lb, g0, g1 ) 
        
        self.assertAlmostEqual(c["f0.A1"], 1.5,10)        
        self.assertAlmostEqual(c["f1.Height"], 7.5,10) 
        self.assertAlmostEqual(c["f2.Height"], 8.5,10) 

        self.assertAlmostEqual(c[0]["A1"], 1.5,10)        
        self.assertAlmostEqual(c[1]["Height"], 7.5,10) 
        self.assertAlmostEqual(c[2]["Height"], 8.5,10)

        self.assertAlmostEqual(c.f0.A1, 1.5,10)        
        self.assertAlmostEqual(c.f1.Height, 7.5,10) 
        self.assertAlmostEqual(c.f2.Height, 8.5,10)         

if __name__ == '__main__':
    unittest.main()
