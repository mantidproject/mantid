# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test of fitfunctions.py and related classes
"""

import unittest
import testhelpers
from mantid.simpleapi import CreateWorkspace, EvaluateFunction, Fit
from mantid.simpleapi import (
    FunctionWrapper,
    CompositeFunctionWrapper,
    ProductFunctionWrapper,
    ConvolutionWrapper,
    MultiDomainFunctionWrapper,
)
from mantid.simpleapi import (
    Gaussian,
    LinearBackground,
    Polynomial,
    MultiDomainFunction,
    Convolution,
    ProductFunction,
    CompositeFunction,
    IFunction1D,
    FunctionFactory,
)
from mantid.api import FunctionDomain1DVector, FunctionDomain1DHistogram
import numpy as np


class FitFunctionsTest(unittest.TestCase):
    _raw_ws = None

    def setUp(self):
        pass

    def test_creation(self):
        testhelpers.assertRaisesNothing(self, FunctionWrapper, "Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)

    def test_name(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        self.assertEqual(g.name, "Gaussian")

    def test_read_array_elements(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        self.assertAlmostEqual(g["Height"], 7.5, 10)

    def test_write_array_elements(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g["Height"] = 8
        self.assertAlmostEqual(g["Height"], 8, 10)

    def test_dot_operator(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        self.assertAlmostEqual(g.Height, 7.5, 10)
        g.Height = 8
        self.assertAlmostEqual(g.Height, 8, 10)

    def test_compositefunction_creation(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, CompositeFunctionWrapper, g0, g1)

    def test_copy_on_compositefunction_creation(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)
        g0["Height"] = 10.0
        g1["Height"] = 11.0
        # Check that the composite function remains unmodified.
        self.assertAlmostEqual(c["f0.Height"], 7.5)
        self.assertAlmostEqual(c["f1.Height"], 8.5)

    def test_getindexoffunction_in_compositefunction(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(lb, g0, g1)
        index_lb = c.getIndexOfFunction("LinearBackground")
        self.assertEqual(index_lb, 0)
        index_g0 = c.getIndexOfFunction("Gaussian 0")
        self.assertEqual(index_g0, 1)
        index_g1 = c.getIndexOfFunction("Gaussian 1")
        self.assertEqual(index_g1, 2)

    def test_compositefunction_f(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(lb, g0, g1)

        lbx = c.f("LinearBackground")
        lbx_str = str(lbx)
        lb_str = str(lb)
        self.assertEqual(lbx_str, lb_str)

        g1x = c.f("Gaussian 1")
        g1x_str = str(g1x)
        g1_str = str(g1)
        self.assertEqual(g1x_str, g1_str)

    def test_compositefunction_read_array_elements(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(lb, g0, g1)

        self.assertAlmostEqual(c["f0.A1"], 1.5, 10)
        self.assertAlmostEqual(c["f1.Height"], 7.5, 10)
        self.assertAlmostEqual(c["f2.Height"], 8.5, 10)

        self.assertAlmostEqual(c[0]["A1"], 1.5, 10)
        self.assertAlmostEqual(c[1]["Height"], 7.5, 10)
        self.assertAlmostEqual(c[2]["Height"], 8.5, 10)

    def test_compositefunction_write_array_elements(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(lb, g0, g1)

        c["f0.A1"] = 0.0
        self.assertAlmostEqual(c["f0.A1"], 0.0, 10)
        c[0]["A1"] = 1.0
        self.assertAlmostEqual(c["f0.A1"], 1.0, 10)

        c["f1.Height"] = 10.0
        self.assertAlmostEqual(c[1]["Height"], 10.0, 10)
        c[1]["Height"] = 11.0
        self.assertAlmostEqual(c[1]["Height"], 11.0, 10)

        g0a = FunctionWrapper("Gaussian", Height=7.0, Sigma=1.2, PeakCentre=9)
        c[1] = g0a
        self.assertAlmostEqual(c[1]["Height"], 7.0, 10)

    def test_attributes(self):
        testhelpers.assertRaisesNothing(self, FunctionWrapper, "Polynomial", attributes={"n": 3}, A0=4, A1=3, A2=2, A3=1)
        testhelpers.assertRaisesNothing(self, FunctionWrapper, "Polynomial", n=3, A0=4, A1=3, A2=2, A3=1)
        p = Polynomial(n=3, A0=1, A1=2, A2=4, A3=3)
        self.assertEqual(p["n"], 3)
        p["n"] = 4
        self.assertEqual(p["n"], 4)

    def test_fix(self):
        g = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)

        g.fix("Sigma")
        g_str = str(g)
        self.assertEqual(g_str.count("ties="), 1)
        self.assertEqual(g_str.count("ties=(Sigma=1.2)"), 1)

        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)

        c.fix("f1.Sigma")
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 1)
        self.assertEqual(c_str.count("ties=(Sigma=1.2"), 1)

        # remove non-existent tie and test it has no effect
        c.untie("f1.Height")
        cu_str = str(c)
        self.assertEqual(c_str, cu_str)

        # remove actual tie
        c.untie("f1.Sigma")
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_fix_all(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)

        c.fixAll("Sigma")
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 2)
        self.assertEqual(c_str.count("ties=(Sigma="), 2)

        # remove non-existent ties and test it has no effect
        c.untieAll("Height")
        cu_str = str(c)
        self.assertEqual(c_str, cu_str)

        # remove actual ties
        c.untieAll("Sigma")
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_fix_all_parameters(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        c = CompositeFunctionWrapper(g0, g1)

        c.fixAllParameters()
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 2)
        self.assertEqual(c_str.count("ties=(Height=7.5,PeakCentre=10,Sigma=1.2)"), 2)

        c.untieAllParameters()
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_fix_all_parameters_with_ties(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        c = CompositeFunctionWrapper(g0, g1)
        c.tie({"f1.Sigma": "f0.Sigma"})

        c.fixAllParameters()
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 3)
        self.assertEqual(c_str.count("ties=(Height=7.5,PeakCentre=10,Sigma=1.2)"), 1)
        self.assertEqual(c_str.count("ties=(Height=7.5,PeakCentre=10)"), 1)
        self.assertEqual(c_str.count("ties=(f1.Sigma=f0.Sigma)"), 1)

        c.untieAllParameters()
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_tie(self):
        g = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)

        g.tie(Sigma="0.1*Height")
        g_str = str(g)
        self.assertEqual(g_str.count("ties="), 1)
        self.assertEqual(g_str.count("ties=(Sigma=0.1*Height)"), 1)

        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)

        c.tie({"f1.Sigma": "f0.Sigma"})
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 1)
        self.assertEqual(c_str.count("ties=(f1.Sigma=f0.Sigma)"), 1)

        c.untie("f1.Sigma")
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_tie_all(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)

        c.tieAll("Sigma")
        c_str = str(c)
        self.assertEqual(c_str.count("ties="), 1)
        self.assertEqual(c_str.count("ties=(f1.Sigma=f0.Sigma)"), 1)

        c.untieAll("Sigma")
        cz_str = str(c)
        self.assertEqual(cz_str.count("ties="), 0)

    def test_constrain(self):
        g = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)

        g.constrain("Sigma < 2.0, Height > 7.0")
        g_str = str(g)
        self.assertEqual(g_str.count("constraints="), 1)
        self.assertEqual(g_str.count("Sigma<2"), 1)
        self.assertEqual(g_str.count("7<Height"), 1)

        g.unconstrain("Height")
        g1_str = str(g)
        self.assertEqual(g1_str.count("constraints="), 1)
        self.assertEqual(g1_str.count("constraints=(Sigma<2)"), 1)

        g.unconstrain("Sigma")
        gz_str = str(g)
        self.assertEqual(gz_str.count("constraints="), 0)

    def test_constrain_composite(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        c = CompositeFunctionWrapper(g0, g1)

        c.constrain("f1.Sigma < 2, f0.Height > 7")
        c_str = str(c)
        self.assertEqual(c_str.count("f1.Sigma<2"), 1)
        self.assertEqual(c_str.count("7<f0.Height"), 1)

        c.unconstrain("f1.Sigma")
        c.unconstrain("f0.Height")
        cz_str = str(c)
        self.assertEqual(cz_str.count("Constraints="), 0)

    def test_constrainall(self):
        lb = FunctionWrapper("LinearBackground", A0=0.5, A1=1.5)
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        g2 = FunctionWrapper("Gaussian", Height=9.5, Sigma=1.2, PeakCentre=12)
        c0 = CompositeFunctionWrapper(g1, g2)
        c = CompositeFunctionWrapper(lb, g0, c0)
        c.constrainAll("Sigma < 1.8")

        c_str = str(c)
        self.assertEqual(c_str.count("constraints="), 3)
        self.assertEqual(c_str.count("Sigma<1.8"), 3)

        lb_str = str(c[0])
        self.assertEqual(lb_str.count("constraints="), 0)

        g0_str = str(c[1])
        self.assertEqual(g0_str.count("constraints="), 1)
        self.assertEqual(g0_str.count("Sigma<1.8"), 1)

        g1_str = str(c[2][0])
        self.assertEqual(g1_str.count("constraints="), 1)
        self.assertEqual(g1_str.count("Sigma<1.8"), 1)

        g2_str = str(c[2][1])
        self.assertEqual(g2_str.count("constraints="), 1)
        self.assertEqual(g2_str.count("Sigma<1.8"), 1)

        c.unconstrainAll("Sigma")

        cz_str = str(c)
        self.assertEqual(cz_str.count("constraints="), 0)

    def test_free(self):
        g = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=15)

        g.constrain("Sigma < 2.0, Height > 7.0")
        g.tie({"PeakCentre": "2*Height"})

        g.free("Height")
        g1_str = str(g)
        self.assertEqual(g1_str.count("ties="), 1)
        self.assertEqual(g1_str.count("constraints="), 1)
        self.assertEqual(g1_str.count("constraints=(Sigma<2)"), 1)

        g.free("PeakCentre")
        g2_str = str(g)
        self.assertEqual(g2_str.count("ties="), 0)
        self.assertEqual(g2_str.count("constraints="), 1)

        g.free("Sigma")
        gz_str = str(g)
        self.assertEqual(gz_str.count("constraints="), 0)

    def test_pureaddition_and_puremultiplication(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)

        sum = CompositeFunctionWrapper(g0, g1)
        self.assertEqual(sum.pureAddition, True)
        self.assertEqual(sum.pureMultiplication, False)

        product = ProductFunctionWrapper(g0, g1)
        self.assertEqual(product.pureAddition, False)
        self.assertEqual(product.pureMultiplication, True)

        sum2 = CompositeFunctionWrapper(sum, g1)
        self.assertEqual(sum2.pureAddition, True)
        self.assertEqual(sum2.pureMultiplication, False)

        product2 = ProductFunctionWrapper(product, g1)
        self.assertEqual(product2.pureAddition, False)
        self.assertEqual(product2.pureMultiplication, True)

        mixed1 = CompositeFunctionWrapper(product, g1)
        self.assertEqual(mixed1.pureAddition, False)
        self.assertEqual(mixed1.pureMultiplication, False)

        mixed2 = CompositeFunctionWrapper(g1, product)
        self.assertEqual(mixed2.pureAddition, False)
        self.assertEqual(mixed2.pureMultiplication, False)

        mixed3 = ProductFunctionWrapper(sum, g1)
        self.assertEqual(mixed3.pureAddition, False)
        self.assertEqual(mixed3.pureMultiplication, False)

        mixed4 = ProductFunctionWrapper(g1, sum)
        self.assertEqual(mixed4.pureAddition, False)
        self.assertEqual(mixed4.pureMultiplication, False)

    def test_flatten(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        g2 = FunctionWrapper("Gaussian", Height=9.5, Sigma=1.3, PeakCentre=14)
        lorentzian = FunctionWrapper("Lorentzian", PeakCentre=9, Amplitude=2.4, FWHM=3)
        lb = FunctionWrapper("LinearBackground")

        # Test already flat composite function, no change should occur
        c1 = CompositeFunctionWrapper(lb, g0, g1)
        fc1 = c1.flatten()
        c1_str = str(c1)
        fc1_str = str(fc1)
        self.assertEqual(fc1_str, c1_str)

        # Test composite function of depth 1
        c2 = CompositeFunctionWrapper(c1, lorentzian)
        fc2 = c2.flatten()
        fc2_str = str(fc2)
        self.assertEqual(fc2_str.count("("), 0)
        self.assertEqual(fc2_str.count("PeakCentre"), 3)
        self.assertEqual(fc2_str.count("Sigma="), 2)
        self.assertEqual(fc2_str.count("Sigma=1.25"), 1)

        # Test composite function of depth 2
        c3 = CompositeFunctionWrapper(g2, c2)
        fc3 = c3.flatten()
        fc3_str = str(fc3)
        self.assertEqual(fc3_str.count("("), 0)
        self.assertEqual(fc3_str.count("PeakCentre"), 4)
        self.assertEqual(fc3_str.count("Sigma="), 3)
        self.assertEqual(fc3_str.count("Sigma=1.25"), 1)
        self.assertEqual(fc3_str.count("Sigma=1.3"), 1)

        # Test product function of depth 1
        p1 = ProductFunctionWrapper(lb, g0, g1)
        p2 = ProductFunctionWrapper(p1, lorentzian)
        fp2 = p2.flatten()
        self.assertTrue(isinstance(fp2, ProductFunctionWrapper))
        fp2_str = str(fp2)
        self.assertEqual(fp2_str.count("("), 0)
        self.assertEqual(fp2_str.count("PeakCentre"), 3)
        self.assertEqual(fp2_str.count("Sigma="), 2)
        self.assertEqual(fp2_str.count("Sigma=1.25"), 1)

    def test_add(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        c = lb + g0 + g1

        self.assertTrue(isinstance(c, CompositeFunctionWrapper))
        c_str = str(c)
        self.assertEqual(c_str.count("("), 0)
        self.assertEqual(c_str.count("LinearBackground"), 1)
        self.assertEqual(c_str.count("Gaussian"), 2)

        lb_str = str(lb)
        c0_str = str(c[0])
        self.assertEqual(c0_str, lb_str)

        g0_str = str(g0)
        c1_str = str(c[1])
        self.assertEqual(c1_str, g0_str)

        g1_str = str(g1)
        c2_str = str(c[2])
        self.assertEqual(c2_str, g1_str)

    def test_incremental_add(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        c = CompositeFunctionWrapper(lb, g0)
        c += g1
        c_str = str(c)
        self.assertEqual(c_str.count("("), 0)
        self.assertEqual(c_str.count("LinearBackground"), 1)
        self.assertEqual(c_str.count("Gaussian"), 2)

        lb_str = str(lb)
        c0_str = str(c[0])
        self.assertEqual(c0_str, lb_str)

        g0_str = str(g0)
        c1_str = str(c[1])
        self.assertEqual(c1_str, g0_str)

        g1_str = str(g1)
        c2_str = str(c[2])
        self.assertEqual(c2_str, g1_str)

    def test_del(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        c = CompositeFunctionWrapper(lb, g0, g1)
        del c[1]

        c_str = str(c)
        self.assertEqual(c_str.count("("), 0)
        self.assertEqual(c_str.count("LinearBackground"), 1)
        self.assertEqual(c_str.count("Gaussian"), 1)
        self.assertEqual(c_str.count("Height=8.5"), 1)

    def test_len(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        c = CompositeFunctionWrapper(lb, g0, g1)
        self.assertEqual(len(c), 3)

    def test_iteration(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        c = CompositeFunctionWrapper(lb, g0, g1)
        count = 0
        for f in c:
            count += 1
        self.assertEqual(count, 3)

    def test_productfunction_creation(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, ProductFunctionWrapper, g0, g1)

    def test_productfunction_creation_by_name(self):
        g0 = Gaussian(Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = Gaussian(Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, ProductFunction, g0, g1)

    def test_mul(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.25, PeakCentre=12)
        lb = FunctionWrapper("LinearBackground")

        p = lb * g0 * g1

        self.assertTrue(isinstance(p, ProductFunctionWrapper))
        p_str = str(p)
        self.assertEqual(p_str.count("("), 0)
        self.assertEqual(p_str.count("LinearBackground"), 1)
        self.assertEqual(p_str.count("Gaussian"), 2)

        lb_str = str(lb)
        p0_str = str(p[0])
        self.assertEqual(p0_str, lb_str)

        g0_str = str(g0)
        p1_str = str(p[1])
        self.assertEqual(p1_str, g0_str)

        g1_str = str(g1)
        p2_str = str(p[2])
        self.assertEqual(p2_str, g1_str)

    def test_convolution_creation(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, ConvolutionWrapper, g0, g1)

    def test_convolution_creation_by_name(self):
        g0 = Gaussian(Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = Gaussian(Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, Convolution, g0, g1)

    def test_multidomainfunction_creation(self):
        g0 = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = FunctionWrapper("Gaussian", Height=8.5, Sigma=1.2, PeakCentre=11)
        testhelpers.assertRaisesNothing(self, MultiDomainFunctionWrapper, g0, g1)
        m = MultiDomainFunctionWrapper(g0, g1, Global=["Height"])
        self.assertEqual(m.nDomains, 2)
        m_str = str(m)
        self.assertEqual(m_str.count("ties"), 1)
        self.assertEqual(m_str.count("Height"), 4)  # 2 in functions 2 in ties

    def test_multidomainfunction_creation_by_name(self):
        g0 = Gaussian(Height=7.5, Sigma=1.2, PeakCentre=10)
        g1 = Gaussian(Height=8.5, Sigma=1.2, PeakCentre=11)
        m = MultiDomainFunction(g0, g1, Global=["Height"])
        self.assertEqual(m.nDomains, 2)
        m_str = str(m)
        self.assertEqual(m_str.count("ties"), 1)
        self.assertEqual(m_str.count("Height"), 4)  # 2 in functions 2 in ties

    def test_generatedfunction(self):
        testhelpers.assertRaisesNothing(self, Gaussian, Height=7.5, Sigma=1.2, PeakCentre=10)
        lb = LinearBackground()
        g = Gaussian(Height=7.5, Sigma=1.2, PeakCentre=10)
        s = lb + g
        self.assertTrue(isinstance(s, CompositeFunctionWrapper))

    def test_direct_evaluation(self):
        l0 = FunctionWrapper("LinearBackground", A0=0, A1=2)
        l1 = FunctionWrapper("LinearBackground", A0=5, A1=-1)

        ws = CreateWorkspace(DataX=[0, 1, 2, 3, 4], DataY=[5, 5, 5, 5], StoreInADS=False)

        c = CompositeFunctionWrapper(l0, l1)
        cws = EvaluateFunction(c, ws, OutputWorkspace="out")
        cvals = cws.readY(1)
        self.assertAlmostEqual(cvals[0], 5.5)
        self.assertAlmostEqual(cvals[1], 6.5)
        self.assertAlmostEqual(cvals[2], 7.5)
        self.assertAlmostEqual(cvals[3], 8.5)

        p = ProductFunctionWrapper(l0, l1)
        pws = EvaluateFunction(p, ws, OutputWorkspace="out")
        pvals = pws.readY(1)
        self.assertAlmostEqual(pvals[0], 4.5)
        self.assertAlmostEqual(pvals[1], 10.5)
        self.assertAlmostEqual(pvals[2], 12.5)
        self.assertAlmostEqual(pvals[3], 10.5)

        sq = Polynomial(attributes={"n": 2}, A0=0, A1=0.0, A2=1.0)
        sqws = EvaluateFunction(sq, ws, OutputWorkspace="out")
        sqvals = sqws.readY(1)
        self.assertAlmostEqual(sqvals[0], 0.25)
        self.assertAlmostEqual(sqvals[1], 2.25)
        self.assertAlmostEqual(sqvals[2], 6.25)

    def test_arithmetic(self):
        l0 = FunctionWrapper("LinearBackground", A0=0, A1=2)
        l1 = FunctionWrapper("LinearBackground", A0=5, A1=-1)

        ws = CreateWorkspace(DataX=[0, 1], DataY=[5], StoreInADS=False)

        c = CompositeFunctionWrapper(l0, l1)
        p = ProductFunctionWrapper(l0, l1)

        s1 = c + p
        s1ws = EvaluateFunction(s1, ws, OutputWorkspace="out")
        s1vals = s1ws.readY(1)
        self.assertAlmostEqual(s1vals[0], 10.0)

        s2 = p + c
        s2ws = EvaluateFunction(s2, ws, OutputWorkspace="out")
        s2vals = s2ws.readY(1)
        self.assertAlmostEqual(s2vals[0], 10.0)

    def test_evaluation_by_single_value(self):
        p = Polynomial(n=4, A0=1, A1=1, A2=1, A3=1, A4=1)
        self.assertAlmostEqual(p(0), 1.0)
        self.assertAlmostEqual(p(1), 5.0)
        self.assertAlmostEqual(p(2), 31.0)

    def test_evaluation_by_list_of_values(self):
        p = Polynomial(n=4, A0=1, A1=1, A2=1, A3=1, A4=1)
        result = p([0, 1, 3])
        self.assertAlmostEqual(result[0], 1.0)
        self.assertAlmostEqual(result[1], 5.0)
        self.assertAlmostEqual(result[2], 121.0)

    def test_evaluation_by_workspace(self):
        ws = CreateWorkspace(DataX=[0, 1, 2, 3], DataY=[5, 5, 5])
        sq = Polynomial(n=2, A0=0, A1=0, A2=1)
        outWs = sq(ws)
        sqvals = outWs.readY(1)
        self.assertAlmostEqual(sqvals[0], 0.25)
        self.assertAlmostEqual(sqvals[1], 2.25)
        self.assertAlmostEqual(sqvals[2], 6.25)

    def test_evaluation_by_1D_numpy_array(self):
        import numpy as np

        a = np.array([0, 1, 3])
        p = Polynomial(n=4, A0=1, A1=1, A2=1, A3=1, A4=1)
        result = p(a)
        self.assertAlmostEqual(result[0], 1.0)
        self.assertAlmostEqual(result[1], 5.0)
        self.assertAlmostEqual(result[2], 121.0)

    def test_evaluation_by_2D_numpy_array(self):
        import numpy as np

        a = np.array([[0, 1], [2, 3]])
        p = Polynomial(n=4, A0=1, A1=1, A2=1, A3=1, A4=1)
        result = p(a)
        self.assertAlmostEqual(result[0][0], 1.0)
        self.assertAlmostEqual(result[0][1], 5.0)
        self.assertAlmostEqual(result[1][0], 31.0)
        self.assertAlmostEqual(result[1][1], 121.0)

    def test_evaluation_with_parameters_set(self):
        p = Polynomial(n=2)
        result = p([0, 1, 2], 0.0, 0.5, 0.5)
        self.assertAlmostEqual(result[0], 0.0)
        self.assertAlmostEqual(result[1], 1.0)
        self.assertAlmostEqual(result[2], 3.0)

    def test_attributes_passed_to_composite_functions(self):
        cf = Gaussian() + LinearBackground()
        self.assertEqual(cf.getAttributeValue("NumDeriv"), False)
        cf = CompositeFunction(Gaussian(), LinearBackground(), NumDeriv=True)
        self.assertEqual(cf.getAttributeValue("NumDeriv"), True)

    def test_attributes_passed_to_convolution(self):
        cf = Convolution(Gaussian(), Gaussian(), NumDeriv=True)
        self.assertEqual(cf.getAttributeValue("NumDeriv"), True)

    def test_attributes_passed_to_product_functions(self):
        pf = Gaussian() * LinearBackground()
        self.assertEqual(pf.getAttributeValue("NumDeriv"), False)
        pf = ProductFunction(Gaussian(), LinearBackground(), NumDeriv=True)
        self.assertEqual(pf.getAttributeValue("NumDeriv"), True)

    def test_attributes_passed_to_multidomain_function(self):
        cf = MultiDomainFunction(Gaussian(), Gaussian(), NumDeriv=True)
        self.assertEqual(cf.getAttributeValue("NumDeriv"), True)

    def test_new_function(self):
        class AFunction(IFunction1D):
            def init(self):
                self.declareParameter("C", 0.0)

            def function1D(self, xvals):
                c = self.getParameterValue("C")
                return c * xvals

        FunctionFactory.subscribe(AFunction)
        fun = AFunction()
        self.assertEqual(str(fun), "name=AFunction,C=0")
        fun.C = 2
        self.assertEqual(fun(2), 4)
        fun = AFunction(C=3)
        self.assertEqual(fun.C, 3)
        self.assertEqual(fun(2), 6)
        self.assertTrue((fun([2, 3, 4]) == np.array([6, 9, 12])).all())
        self.assertTrue((fun(np.array([2, 3, 4])) == np.array([6, 9, 12])).all())

    def test_new_function_init(self):
        class BFunction(IFunction1D):
            def __init__(self):
                super(BFunction, self).__init__()

            def init(self):
                self.declareParameter("C", 0.0)

            def function1D(self, xvals):
                c = self.getParameterValue("C")
                return c * xvals

        FunctionFactory.subscribe(BFunction)
        fun = BFunction()
        self.assertEqual(str(fun), "name=BFunction,C=0")
        fun.C = 2
        self.assertEqual(fun(2), 4)
        fun = BFunction(C=3)
        self.assertEqual(fun.C, 3)
        self.assertEqual(fun(2), 6)

    def test_multi_domain_fit(self):
        x = np.linspace(-10, 10)
        y1 = np.exp(-((x - 2) ** 2))
        y2 = 2 * np.exp(-(x**2))
        y3 = 3 * np.exp(-((x + 2) ** 2))

        ws1 = CreateWorkspace(x, y1)
        ws2 = CreateWorkspace(x, y2)
        ws3 = CreateWorkspace(x, y3)

        mdf = MultiDomainFunction(Gaussian(), Gaussian(), Gaussian(), Global=["Sigma"])
        res = Fit(mdf, InputWorkspace=ws1, InputWorkspace_1=ws2, InputWorkspace_2=ws3)
        f = res.Function

        self.assertAlmostEqual(f[0].Sigma, 0.707107, 6)
        self.assertAlmostEqual(f[1].Sigma, 0.707107, 6)
        self.assertAlmostEqual(f[2].Sigma, 0.707107, 6)

        self.assertAlmostEqual(f[0].Height, 1, 6)
        self.assertAlmostEqual(f[1].Height, 2, 6)
        self.assertAlmostEqual(f[2].Height, 3, 6)

        self.assertAlmostEqual(f[0].PeakCentre, 2, 6)
        self.assertAlmostEqual(f[1].PeakCentre, 0, 6)
        self.assertAlmostEqual(f[2].PeakCentre, -2, 6)

    def test_plugins_wrapped(self):
        from mantid.simpleapi import Lorentz

        self.assertEqual(str(Lorentz()), "name=Lorentz,Scale=1,Length=50,Background=0")

    def test_function_deriv_with_1DVector_Domain(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        ws = CreateWorkspace(DataX=[0, 1, 2, 3, 4], DataY=[5, 5, 5, 5])

        dom = FunctionDomain1DVector(ws.readX(0))
        out = g.functionDeriv(dom)

        self.assertAlmostEqual(out.get(0, 0), 0.0000000000000008323969676981107)
        self.assertAlmostEqual(out.get(0, 1), -0.000000000000043354008734276606)
        self.assertAlmostEqual(out.get(0, 2), -0.00000000000031214886288679154)

        self.assertAlmostEqual(out.get(1, 0), 0.0000000000006101936677605303)
        self.assertAlmostEqual(out.get(1, 1), -0.00000000002860282817627486)
        self.assertAlmostEqual(out.get(1, 2), -0.00000000018534632658226106)

        self.assertAlmostEqual(out.get(2, 0), 0.00000000022336314362031582)
        self.assertAlmostEqual(out.get(2, 1), -0.000000009306797650846493)
        self.assertAlmostEqual(out.get(2, 2), -0.0000000536071544688758)

    def test_function_deriv_with_1DHistogram_Domain(self):
        g = FunctionWrapper("Gaussian", Height=7.5, Sigma=1.2, PeakCentre=10)
        ws = CreateWorkspace(DataX=[0, 1, 2, 3, 4], DataY=[5, 5, 5, 5])

        dom = FunctionDomain1DHistogram(ws.readX(0))
        out = g.functionDeriv(dom)

        self.assertAlmostEqual(out.get(0, 0), 0.00000000000009592326932761353)
        self.assertAlmostEqual(out.get(0, 1), -0.000000000004570209530946241)
        self.assertAlmostEqual(out.get(0, 2), -0.00000000003012829985493681)

        self.assertAlmostEqual(out.get(1, 0), 0.00000000003925992864139971)
        self.assertAlmostEqual(out.get(1, 1), -0.0000000016706471246441646)
        self.assertAlmostEqual(out.get(1, 2), -0.000000009831636305079881)

        self.assertAlmostEqual(out.get(2, 0), 0.000000008131882500705956)
        self.assertAlmostEqual(out.get(2, 1), -0.0000003045374795085354)
        self.assertAlmostEqual(out.get(2, 2), -0.0000015775749015123351)


if __name__ == "__main__":
    unittest.main()
