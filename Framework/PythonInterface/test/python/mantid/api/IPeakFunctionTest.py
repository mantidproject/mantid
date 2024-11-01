# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import FunctionFactory, IFunction1D, IPeakFunction
from mantid.simpleapi import LoadEmptyInstrument, LoadParameterFile
import numpy as np


class MyPeak(IPeakFunction):
    def init(self):
        self.declareAttribute("Centre", 1)
        self.declareAttribute("Height", 3.4)
        self.declareAttribute("Width", 1.5)

    def functionLocal(self, xvals):
        return 5 * xvals


class RectangularFunction(IPeakFunction):
    def init(self):
        if not self.hasParameter("Height"):
            self.declareParameter("Height")
        if not self.hasParameter("Fwhm"):
            self.declareParameter("Fwhm")
        if not self.hasParameter("Center"):
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

    def functionLocal(self, xvals):
        center = self.getParameterValue("Center")
        fwhm = self.getParameterValue("Fwhm")
        height = self.getParameterValue("Height")

        values = np.zeros(xvals.shape)
        nonZero = (xvals > (center - fwhm / 2.0)) & (xvals < (center + fwhm / 2.0))
        values[nonZero] = height

        return values

    def intensityErrorLocal(self):
        r"""Analytical expression for the error in the integrated intensity arising from errors in
        uncorrelated fit paramaters"""
        height = self.getParameterValue("Height")
        height_error = self.getError("Height")
        fwhm = self.getParameterValue("Fwhm")
        fwhm_error = self.getError("Fwhm")
        intensity = height * fwhm
        return intensity * np.sqrt((height_error / height) ** 2 + (fwhm_error / fwhm) ** 2)


FunctionFactory.subscribe(RectangularFunction)


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
        xvals = np.array([1.0, 2.0, 3.0])
        out = func.functionLocal(xvals)
        self.assertEqual(3, out.shape[0])
        self.assertEqual(5.0, out[0])
        self.assertEqual(10.0, out[1])
        self.assertEqual(15.0, out[2])

    def test_get_set_intensity(self):
        func = RectangularFunction()
        func.initialize()
        func.setCentre(1.0)
        func.setHeight(2.0)
        func.setFwhm(3.0)

        # This is a rectangle function with height 2 and width 3, centered
        # around 1.0. The intensity should be 6.0 (height * width)
        self.assertAlmostEqual(func.intensity(), 6.0, places=10)

        # Setting the intensity only changes height, not width
        func.setIntensity(12.0)

        self.assertEqual(func.fwhm(), 3.0)
        self.assertAlmostEqual(func.height(), 4.0, places=10)
        self.assertAlmostEqual(func.intensity(), 12.0, places=10)

    def test_get_set_matrix_workspace(self):
        ws = LoadEmptyInstrument(InstrumentName="ENGIN-X", OutputWorkspace="ws")
        LoadParameterFile(Workspace=ws, Filename="ENGIN-X_Parameters.xml")
        axis = ws.getAxis(0)
        axis.setUnit("TOF")

        func = FunctionFactory.Instance().createPeakFunction("BackToBackExponential")
        func.setParameter(3, 24000)  # set centre
        func.setMatrixWorkspace(ws, 800, 0.0, 0.0)  # calculate A,B,S
        self.assertTrue(func.isExplicitlySet(1))

    def test_intensityError(self):
        func = RectangularFunction()
        func.initialize()
        func.setCentre(1.0)
        func.setHeight(2.0)
        func.setFwhm(3.0)
        # shifting the box doesn't change its area
        func.setError("Center", 0.1)
        self.assertEqual(func.intensityError(), 0.0, 1e-6)
        # general case
        func.setError("Center", 0.1)
        func.setError("Height", 0.2)
        func.setError("Fwhm", 0.3)
        self.assertAlmostEqual(func.intensityError(), func.intensityErrorLocal(), places=6)


if __name__ == "__main__":
    unittest.main()
