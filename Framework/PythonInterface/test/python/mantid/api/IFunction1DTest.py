# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class NoCatgeoryFunction(IFunction1D):
    def init(self):
        pass

    def function1D(self, xvals):
        return xvals


class Times2(IFunction1D):
    def category(self):
        return "SimpleFunction"

    def init(self):
        self.declareAttribute("IntAtt", 1)
        self.declareAttribute("DoubleAtt", 3.4)
        self.declareAttribute("StringAtt", "filename")
        self.declareAttribute("BoolAtt", True)
        self.declareAttribute("ListAtt", [1, 2, 3])

        self.declareParameter("ParamZeroInitNoDescr")
        self.declareParameter("ParamNoDescr", 1.5)
        self.declareParameter("OtherParam", 4, "Some fitting parameter")

    def function1D(self, xvals):
        return 2 * xvals


class IFunction1DTest(unittest.TestCase):
    def test_instance_can_be_created_standalone(self):
        func = Times2()
        self.assertTrue(isinstance(func, IFunction1D))

    def test_instance_can_be_created_from_factory(self):
        FunctionFactory.subscribe(Times2)
        func_name = Times2.__name__
        func = FunctionFactory.createFunction(func_name)
        self.assertTrue(isinstance(func, IFunction1D))
        FunctionFactory.unsubscribe(func_name)

    def test_category_with_no_override_returns_default_category(self):
        FunctionFactory.subscribe(NoCatgeoryFunction)
        func = FunctionFactory.createFunction("NoCatgeoryFunction")
        self.assertEqual("General", func.category())
        FunctionFactory.unsubscribe("NoCatgeoryFunction")

    def test_category_override_returns_overridden_result(self):
        FunctionFactory.subscribe(Times2)
        func = FunctionFactory.createFunction("Times2")
        self.assertEqual("SimpleFunction", func.category())
        FunctionFactory.unsubscribe("Times2")

    def test_declareAttribute_only_accepts_known_types(self):
        func = Times2()
        func.initialize()  # Contains known types
        self.assertEqual(5, func.nAttributes())  # Make sure initialize ran
        self.assertRaises(ValueError, func.declareAttribute, "DictAtt", {1, 2, 3})

    def test_correct_attribute_values_are_returned_when_asked(self):
        func = Times2()
        func.initialize()  # Contains known types

        self.assertEqual(1, func.getAttributeValue("IntAtt"))
        self.assertEqual(3.4, func.getAttributeValue("DoubleAtt"))
        self.assertEqual("filename", func.getAttributeValue("StringAtt"))
        self.assertEqual(True, func.getAttributeValue("BoolAtt"))

    def test_correct_parameters_are_attached_during_init(self):
        func = Times2()
        func.initialize()

        self.assertEqual(3, func.nParams())

        self.assertEqual("ParamZeroInitNoDescr", func.parameterName(0))
        self.assertEqual("", func.paramDescription(0))
        self.assertEqual(0.0, func.getParameterValue(0))

        self.assertEqual("ParamNoDescr", func.parameterName(1))
        self.assertEqual("", func.paramDescription(1))
        self.assertEqual(1.5, func.getParameterValue(1))

        self.assertEqual("OtherParam", func.parameterName(2))
        self.assertEqual("Some fitting parameter", func.paramDescription(2))
        self.assertEqual(4.0, func.getParameterValue(2))

    def test_function1D_can_be_called_directly(self):
        func = Times2()
        func.initialize()
        xvals = np.array([1, 2, 3])
        out = func.function1D(xvals)
        self.assertEqual(3, out.shape[0])
        self.assertEqual(2, out[0])
        self.assertEqual(4, out[1])
        self.assertEqual(6, out[2])


if __name__ == "__main__":
    unittest.main()
