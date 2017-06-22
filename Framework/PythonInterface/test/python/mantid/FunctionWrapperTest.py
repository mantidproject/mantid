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
        
    def test_compositefunction_write_array_elements(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper( lb, g0, g1 )
             
        c["f0.A1"] = 0.0
        self.assertAlmostEqual(c["f0.A1"], 0.0,10) 
        c[0]["A1"] = 1.0
        self.assertAlmostEqual(c["f0.A1"], 1.0,10)
        
        c["f1.Height"] = 10.0
        self.assertAlmostEqual(c[1]["Height"], 10.0,10) 
        c[1]["Height"] = 11.0
        self.assertAlmostEqual(c[1]["Height"], 11.0,10)

    def test_fix(self):
        g = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)
        
        g.fix("Sigma")
        g_str = g.__str__()
        self.assertEqual(g_str.count("ties="),1)
        self.assertEqual(g_str.count("ties=(Sigma=1.2)"),1)
        
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.fix("f1.Sigma")
        c_str = c.__str__()
        self.assertEqual(c_str.count("ties="),1)
        self.assertEqual(c_str.count("ties=(Sigma=1.2)"),1)

    def test_tie(self):
        g = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)
        
        g.tie(Sigma="0.1*Height")
        g_str = g.__str__()
        self.assertEqual(g_str.count("ties="),1)
        self.assertEqual(g_str.count("ties=(Sigma=0.1*Height)"),1)
        
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.tie({"f1.Sigma":"f0.Sigma"})
        c_str = c.__str__()
        self.assertEqual(c_str.count("ties="),1)
        self.assertEqual(c_str.count("ties=(f1.Sigma=f0.Sigma)"),1)

        
if __name__ == '__main__':
    unittest.main()
