# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import IFunction1D
from mantid.kernel import (
    FloatArrayBoundedValidator,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringContainsValidator,
    StringListValidator,
)


class SimpleFuncWValidator(IFunction1D):
    def category(self):
        pass

    def init(self, input_case):
        if input_case == "String List":
            self.declareAttribute("StringAtt", "filename", StringListValidator(["filename", "test"]))
        elif input_case == "Int Bounded":
            self.declareAttribute("IntAtt", 3, IntBoundedValidator(lower=0, upper=3))
        elif input_case == "Float Bounded":
            self.declareAttribute("FloatAtt", 3.0, FloatBoundedValidator(0.0, 5.0))
        elif input_case == "Array Bounded":
            self.declareAttribute("ListAtt", [1.0, 2.0, 3.0], FloatArrayBoundedValidator(0.0, 5.0))
        elif input_case == "String Contains":
            self.declareAttribute("StringContainsAtt", "StringContains", StringContainsValidator(["Contains"]))

    def invalid_init(self, input_case):
        if input_case == "String List":
            self.declareAttribute("StringAtt", "error", StringListValidator(["filename", "test"]))
        elif input_case == "Int Bounded":
            self.declareAttribute("IntAtt", 4, IntBoundedValidator(lower=0, upper=3))
        elif input_case == "Float Bounded":
            self.declareAttribute("FloatAtt", 10.0, FloatBoundedValidator(0.0, 5.0))
        elif input_case == "Array Bounded":
            self.declareAttribute("ListAtt", [1.0, 2.0, 3.0, 10.0], FloatArrayBoundedValidator(0.0, 5.0))
        elif input_case == "String Contains":
            self.declareAttribute("StringContainsAtt", "error", StringContainsValidator(["Contains"]))

    def function1D(self, xvals):
        pass


class IFunction1DValidatorTest(unittest.TestCase):
    def test_int_bounded_validator(self):
        func = SimpleFuncWValidator()

        self.assertRaises(Exception, func.invalid_init, "Int Bounded")
        func.init("Int Bounded")

        self.assertRaises(Exception, func.setAttributeValue, "IntAtt", 4)

    def test_float_bounded_validator(self):
        func = SimpleFuncWValidator()

        self.assertRaises(Exception, func.invalid_init, "Float Bounded")
        func.init("Float Bounded")

        self.assertRaises(Exception, func.setAttributeValue, "FloatAtt", 10.0)

    def test_list_validator(self):
        func = SimpleFuncWValidator()

        self.assertRaises(Exception, func.invalid_init, "String List")
        func.init("String List")

        self.assertRaises(Exception, func.setAttributeValue, "StringAtt", "error")

    def test_array_bounded_validator(self):
        func = SimpleFuncWValidator()

        self.assertRaises(Exception, func.invalid_init, "Array Bounded")
        func.init("Array Bounded")

        invalid_input_vals = [1.0, 2.0, 3.0, 10.0]

        self.assertRaises(Exception, func.setAttributeValue, "ListAtt", invalid_input_vals)

    def test_string_contains_validator(self):
        func = SimpleFuncWValidator()

        self.assertRaises(Exception, func.invalid_init, "String Contains")
        func.init("String Contains")

        self.assertRaises(Exception, func.setAttributeValue, "StringContainsAtt", "error")


if __name__ == "__main__":
    unittest.main()
