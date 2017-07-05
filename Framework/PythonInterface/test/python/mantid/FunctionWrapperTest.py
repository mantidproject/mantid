"""
    Test of FunctiomWrapper and related classes
"""
from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers
import platform
from mantid.simpleapi import CreateWorkspace, Fit, FitDialog, FunctionWrapper, CompositeFunctionWrapper, ProductFunctionWrapper, ConvolutionWrapper, MultiDomainFunctionWrapper, Gaussian
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
        gz_str = g.__str__()
        self.assertEqual(gz_str.count("constraints="),0)
        
    def test_constrain_composite(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        
        c.constrain("f1.Sigma < 2, f0.Height > 7")
        c_str = c.__str__()
        self.assertEqual(c_str.count("constraints="),2)
        self.assertEqual(c_str.count("Sigma<2"),1)
        self.assertEqual(c_str.count("7<Height"),1)
        
        g0_str = c[0].__str__()
        self.assertEqual(g0_str.count("constraints="),1)
        self.assertEqual(g0_str.count("7<Height"),1)
        
        g1_str = c[1].__str__()
        self.assertEqual(g1_str.count("constraints="),1)
        self.assertEqual(g1_str.count("Sigma<2"),1)
        
        c[1].unconstrain("Sigma")
        c[0].unconstrain("Height")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("Constraints="),0)
        
        c[1].constrain("Sigma < 3")
        c1_str = c.__str__()
        self.assertEqual(c1_str.count("constraints="),1)
        self.assertEqual(c1_str.count("Sigma<3"),1)
        
        c.unconstrain("f1.Sigma")
        c.unconstrain("f0.Height")
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("Constraints="),0)
        
    def test_constrainall(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        g2 = FunctionWrapper( "Gaussian", Height=9.5, Sigma=1.2, PeakCentre=12)
        c0 = CompositeFunctionWrapper( g1, g2 )
        c = CompositeFunctionWrapper( lb, g0, c0)
        c.constrainAll("Sigma < 1.8")
        
        c_str = c.__str__()
        self.assertEqual(c_str.count("constraints="),3)
        self.assertEqual(c_str.count("Sigma<1.8"),3)
        
        lb_str = c[0].__str__()
        self.assertEqual(lb_str.count("constraints="),0)
        
        g0_str = c[1].__str__()
        self.assertEqual(g0_str.count("constraints="),1)
        self.assertEqual(g0_str.count("Sigma<1.8"),1)
        
        g1_str = c[2][0].__str__()
        self.assertEqual(g1_str.count("constraints="),1)
        self.assertEqual(g1_str.count("Sigma<1.8"),1)
        
        g2_str = c[2][1].__str__()
        self.assertEqual(g2_str.count("constraints="),1)
        self.assertEqual(g2_str.count("Sigma<1.8"),1)
        
        c.unconstrainAll("Sigma")
        
        cz_str = c.__str__()
        self.assertEqual(cz_str.count("constraints="),0)
              
    def test_free(self):
        g = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)
        
        g.constrain("Sigma < 2.0, Height > 7.0")
        g.tie({"PeakCentre":"2*Height"})
        
        g.free("Height")
        g1_str = g.__str__()
        self.assertEqual(g1_str.count("ties="),1)
        self.assertEqual(g1_str.count("constraints="),1)
        self.assertEqual(g1_str.count("constraints=(Sigma<2)"),1)
        
        g.free("PeakCentre")
        g2_str = g.__str__()
        self.assertEqual(g2_str.count("ties="),0)
        self.assertEqual(g2_str.count("constraints="),1)
        
        g.free("Sigma")
        gz_str = g.__str__()
        self.assertEqual(gz_str.count("constraints="),0)
        
    def test_flatten(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)  
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)  
        g2 = FunctionWrapper( "Gaussian", Height=9.5, Sigma=1.3, PeakCentre=14)
        l = FunctionWrapper("Lorentzian",PeakCentre=9, Amplitude=2.4, FWHM=3)
        lb = FunctionWrapper("LinearBackground")
        
        # Test already flat composite function, no change should occur
        c1 = CompositeFunctionWrapper(lb, g0, g1 )
        fc1 = c1.flatten()
        c1_str = c1.__str__()
        fc1_str = fc1.__str__()
        self.assertEqual(fc1_str,c1_str)
        
        # Test composite function of depth 1
        c2 = CompositeFunctionWrapper(c1, l)
        fc2 = c2.flatten()
        fc2_str = fc2.__str__()
        self.assertEqual(fc2_str.count("("),0)
        self.assertEqual(fc2_str.count("PeakCentre"),3)
        self.assertEqual(fc2_str.count("Sigma="),2)
        self.assertEqual(fc2_str.count("Sigma=1.25"),1)
        
        # Test composite function of depth 2
        c3 = CompositeFunctionWrapper( g2, c2)
        fc3 = c3.flatten()
        fc3_str = fc3.__str__()
        self.assertEqual(fc3_str.count("("),0)
        self.assertEqual(fc3_str.count("PeakCentre"),4)
        self.assertEqual(fc3_str.count("Sigma="),3)
        self.assertEqual(fc3_str.count("Sigma=1.25"),1)
        self.assertEqual(fc3_str.count("Sigma=1.3"),1)
        
    def test_add(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)  
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)  
        lb = FunctionWrapper("LinearBackground")
        
        c = lb + g0 + g1
        
        c_str = c.__str__()
        self.assertEqual(c_str.count("("),0)
        self.assertEqual(c_str.count("LinearBackground"),1)
        self.assertEqual(c_str.count("Gaussian"),2)
        
        lb_str = lb.__str__()
        c0_str = c[0].__str__()
        self.assertEqual(c0_str, lb_str)
           
        g0_str = g0.__str__()
        c1_str = c[1].__str__()
        self.assertEqual(c1_str, g0_str)
        
        g1_str = g1.__str__()
        c2_str = c[2].__str__()
        self.assertEqual(c2_str, g1_str)
        
    def test_productfunction_creation(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, ProductFunctionWrapper, g0, g1)

    def test_mul(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)  
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)  
        lb = FunctionWrapper("LinearBackground")
        
        p = lb * g0 * g1
        
        p_str = p.__str__()
        self.assertEqual(p_str.count("("),0)
        self.assertEqual(p_str.count("LinearBackground"),1)
        self.assertEqual(p_str.count("Gaussian"),2)
        
        lb_str = lb.__str__()
        p0_str = p[0].__str__()
        self.assertEqual(p0_str, lb_str)
           
        g0_str = g0.__str__()
        p1_str = p[1].__str__()
        self.assertEqual(p1_str, g0_str)
        
        g1_str = g1.__str__()
        p2_str = p[2].__str__()
        self.assertEqual(p2_str, g1_str)   

    def test_convolution_creation(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, ConvolutionWrapper, g0, g1)
        
    def test_multidomainfunction_creation(self):
        g0 = FunctionWrapper( "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper( "Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, MultiDomainFunctionWrapper, g0, g1)
        m = MultiDomainFunctionWrapper( g0, g1)
        self.assertEqual( m.nDomains(), 2)
        
    def test_prefinedfunction(self):
        testhelpers.assertRaisesNothing(self, Gaussian, Height=7.5, Sigma=1.2, PeakCentre=10)
       
if __name__ == '__main__':
    unittest.main()
