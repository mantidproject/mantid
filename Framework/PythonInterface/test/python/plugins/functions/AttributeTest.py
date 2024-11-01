# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import FunctionFactory, IFunction1D
import numpy


class AttributeExample(IFunction1D):
    def init(self):
        self.declareParameter("Amplitude", 0.2)
        self.declareParameter("Baseline", 0.1)
        self.declareAttribute("Frequency", 0.26)
        self.declareAttribute("Sine", False)

    def setAttributeValue(self, name, value):
        if name == "Frequency":
            self._freq = value
        if name == "Sine":
            self._sine = value

    def function1D(self, xvals):
        ampl = self.getParameterValue("Amplitude")
        base = self.getParameterValue("Baseline")

        if self._sine:
            ybins = numpy.sin(xvals * self._freq * 2.0 * numpy.pi) * ampl + base
        else:
            ybins = numpy.cos(xvals * self._freq * 2.0 * numpy.pi) * ampl + base
        return ybins


FunctionFactory.subscribe(AttributeExample)


class AttributeTest(unittest.TestCase):
    def test_setAttributeValue_called_on_declaration(self):
        fun = FunctionFactory.createFunction("AttributeExample")
        res = fun.function1D(numpy.array([0, 1, 2, 3]))
        # If function1D doesn't throw an exception then _freq and _sine
        # member variables are defined.
        # res[0] == 0.3 means that _sin is False
        self.assertAlmostEqual(res[0], 0.3, 10)
        self.assertAlmostEqual(res[1], 0.0874419, 7)
        self.assertAlmostEqual(res[2], -0.09842294, 7)
        self.assertAlmostEqual(res[3], 0.13747626, 7)

    def test_setAttributeValue_called(self):
        fun = FunctionFactory.createInitialized("name=AttributeExample,Sine=true")
        res = fun.function1D(numpy.array([0, 1, 2, 3]))
        # res[0] == 0.1 means that _sin is True
        self.assertAlmostEqual(res[0], 0.1, 10)
        self.assertAlmostEqual(res[1], 0.29960535, 7)
        self.assertAlmostEqual(res[2], 0.07493335, 7)
        self.assertAlmostEqual(res[3], -0.09645745, 7)

    def test_set_and_get_attribute_value(self):
        fun = FunctionFactory.createFunction("AttributeExample")
        self.assertEqual(fun.getAttributeValue("Frequency"), 0.26)
        fun.setAttributeValue("Frequency", 6.2)
        self.assertEqual(fun.getAttributeValue("Frequency"), 6.2)


if __name__ == "__main__":
    unittest.main()
