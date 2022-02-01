# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import IFunction1D, IFunction, FunctionFactory
from mantid.kernel import StringListValidator, IntBoundedValidator, StringContainsValidator, FloatArrayBoundedValidator


class SimpleFuncWValidator(IFunction1D):

    def category(self):
        return "SimpleFunctionWithValidator"

    def init(self, input_case):
        if input_case == "String List":
            self.declareAttribute("StringAtt", "filename", StringListValidator(["filename","test"]))
        elif input_case == "Int Bounded":
            self.declareAttribute("IntAtt", 1, IntBoundedValidator(0, 5))
        elif input_case == "Array Bounded":
            self.declareAttribute("ListAtt", [1.0, 2.0, 3.0], FloatArrayBoundedValidator(0.0, 5.0))
        elif input_case == "String Contains":
            self.declareAttribute("StringContainsAtt", "StringContains", StringContainsValidator("Contains"))

    def function1D(self, xvals):
        return xvals


class IFunction1DValidatorTest(unittest.TestCase):

    def test_bounded_validator(self):
        func = SimpleFuncWValidator()
        func.init("Int Bounded")

        self.assertRaises(Exception, func.setAttributeValue, "IntAtt", 10)

    def test_list_validator(self):
        func = SimpleFuncWValidator()
        func.init("String List")

        self.assertRaises(Exception, func.setAttributeValue,"StringAtt", "error")

    def test_array_bounded_validator(self):
        func = SimpleFuncWValidator()
        func.init("Array Bounded")

        invalid_input_vals = [1.0, 2.0, 3.0, 10.0]

        self.assertRaises(Exception, func.setAttributeValue,"ListAtt", invalid_input_vals)

    def test_string_contains_validator(self):
        func = SimpleFuncWValidator()
        func.init("String Contains")

        self.assertRaises(Exception, func.setAttributeValue,"StringContainsAtt", "error")


if __name__ == '__main__':
    unittest.main()
