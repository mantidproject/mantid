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
        self.assertEqual(c_str.count("ties=(Sigma=1.2"),1)
        
        # remove non-existent tie and test it has no effect
        c.untie("f1.Height")
        cu_str = c.__str__()
        self.assertEqual(c_str, cu_str)
        
        # remove actual tie
        c.untie("f1.Sigma")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("ties="),0)
        
    def test_fix_all(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.fixAll("Sigma")
        c_str = c.__str__()
        self.assertEqual(c_str.count("ties="),2)
        self.assertEqual(c_str.count("ties=(Sigma="),2)
        
        # remove non-existent ties and test it has no effect
        c.untieAll("Height")
        cu_str = c.__str__()
        self.assertEqual(c_str, cu_str)
        
        # remove actual ties
        c.untieAll("Sigma")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("ties="),0)
        
    def test_fix_all_parameters(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.fixAllParameters()
        c_str = c.__str__()
        self.assertEqual(c_str.count("ties="),2)
        self.assertEqual(c_str.count("ties=(Height=7.5,PeakCentre=10,Sigma=1.2)"),2)
        
        c.untieAllParameters()
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("ties="),0)
    

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
        
        c.untie("f1.Sigma")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("ties="),0)
        
    def test_tie_all(self):
    
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.tieAll("Sigma")
        c_str = c.__str__()
        self.assertEqual(c_str.count("ties="),1)
        self.assertEqual(c_str.count("ties=(f1.Sigma=f0.Sigma)"),1)
        
        c.untieAll("Sigma")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("ties="),0)
        
    def test_constrain(self):
        g = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)
        
        g.constrain("Sigma < 2.0, Height > 7.0")
        g_str = g.__str__()
        self.assertEqual(g_str.count("constraints="),1)
        self.assertEqual(g_str.count("constraints=(7<Height,Sigma<2)"),1)
        
        g.unconstrain("Height")
        g1_str = g.__str__()
        self.assertEqual(g1_str.count("constraints="),1)
        self.assertEqual(g1_str.count("constraints=(Sigma<2)"),1)
        
        g.unconstrain("Sigma")
        g1_str = g.__str__()
        self.assertEqual(g1_str.count("constraints="),0)
       
if __name__ == '__main__':
    unittest.main()
