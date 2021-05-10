# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import *
import numpy as np


class MyPeak(IPeakFunction):

    def init(self):
        self.declareAttribute("Centre", 1)
        self.declareAttribute("Height", 3.4)
        self.declareAttribute("Width", 1.5)

    def functionLocal(self, xvals):
        return 5*xvals


class RectangularFunction(IPeakFunction):
    def init(self):
        self.declareParameter("Height")
        self.declareParameter("Fwhm")
        self.declareParameter("Center")

    def centre(self):
        return self.getParameterValue("Center")

    def setCentre(self, newCenter):
        self.setParameter("Center", newCenter)

    def height(self):
        return self.getParameterValue("Height")

    def setHeight(self, newHeight):
        self.setParameter("Height", newHeight)

    def fwhm(self):
        return self.getParameterValue("Fwhm")

    def setFwhm(self, newFwhm):
        self.setParameter("Fwhm", newFwhm)

    def heightError(self):
        return self.getParameterValue("Height_error")

    def setHeightError(self, newHeight):
        self.setParameter("Height_error", newHeight)

    def fwhmError(self):
        return self.getParameterValue("Fwhm_error")

    def setFwhmError(self, newFwhm):
        self.setParameter("Fwhm_error", newFwhm)

    def functionLocal(self, xvals):
        center = self.getParameterValue("Center")
        fwhm = self.getParameterValue("Fwhm")
        height = self.getParameterValue("Height")

        values = np.zeros(xvals.shape)
        nonZero = (xvals > (center - fwhm/2.0)) & (xvals < (center + fwhm / 2.0))
        values[nonZero] = height

        return values


class IPeakFunctionTest(unittest.TestCase):

    def test_instance_can_be_created_standalone(self):
        func = MyPeak()
        self.assertTrue(isinstance(func, IPeakFunction))
        self.assertTrue(isinstance(func, IFunction1D))

    def test_instance_can_be_created_from_factory(self):
        FunctionFactory.subscribe(MyPeak)
        func_name = MyPeak.__name__
        func = FunctionFactory.createFunction(func_name)
        self.assertTrue(isinstance(func, IPeakFunction))
        self.assertTrue(isinstance(func, IFunction1D))
        FunctionFactory.unsubscribe(func_name)

    def test_functionLocal_can_be_called_directly(self):
        func = MyPeak()
        func.initialize()
        xvals=np.array([1.,2.,3.])
        out = func.functionLocal(xvals)
        self.assertEqual(3, out.shape[0])
        self.assertEqual(5., out[0])
        self.assertEqual(10., out[1])
        self.assertEqual(15., out[2])

    def test_get_set_intensity(self):
        func = RectangularFunction()
        func.initialize()
        func.setCentre(1.0)
        func.setHeight(2.0)
        func.setFwhm(3.0)

        # This is a rectangle function with height 2 and width 3, centered
        # around 1.0. The intensity should be 6.0 (height * width)
        self.assertAlmostEquals(func.intensity(), 6.0, places=10)

        # Setting the intensity only changes height, not width
        func.setIntensity(12.0)

        self.assertEqual(func.fwhm(), 3.0)
        self.assertAlmostEquals(func.height(), 4.0, places=10)
        self.assertAlmostEquals(func.intensity(), 12.0, places=10)

    def test_get_set_intensityError(self):
        func = RectangularFunction()
        func.initialize()
        func.setCentre(1.0)
        func.setHeight(2.0)
        func.setFwhm(3.0)
        func.setFwhmError(5.0)
        func.setHeightError(7.0)

        # This is a rectangle function with height 2 and width 3, centered
        # around 1.0. The intensity error should be 31.0 (Height * Fwhm_error + Height_error * Fwhm)
        self.assertAlmostEquals(func.intensityError(), 31.0, places=10)

if __name__ == '__main__':
    unittest.main()
