from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers

from mantid.kernel import StringContainsValidator
from mantid.api import PythonAlgorithm

class StringContainsValidatorTest(unittest.TestCase):


    def test_StringContainsValidator_with_empty_required_string(self):
        """
            Test that a list validator restricts the values
            for a property
        """

        class StringContainsValidatorWithEmptyItem(PythonAlgorithm):

            def PyInit(self):
                validator = StringContainsValidator()
                validator.setRequiredStrings([""])
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = StringContainsValidatorWithEmptyItem()
        alg.initialize()
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", "AnyOldString")

    def test_StringContainsValidator_with_single_required_string(self):
        """
            Test that a string-contains validator that requires
            a single string to be contained in the input string
        """

        class StringContainsValidatorWithSingleItem(PythonAlgorithm):

            def PyInit(self):
                validator = StringContainsValidator()
                validator.setRequiredStrings(["meOw"])
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = StringContainsValidatorWithSingleItem()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "")
        self.assertRaises(ValueError, alg.setProperty, "Input", "NotValid")
        self.assertRaises(ValueError, alg.setProperty, "Input", "Homeowner")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", "HomeOwner")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", "cat, meOws")
        
    def test_StringContainsValidator_with_multiple_required_strings(self):
        """
            Test that a string-contains validator that requires
            multiple strings to all be contained in the input string
        """

        class StringContainsValidatorWithMultipleItem(PythonAlgorithm):

            def PyInit(self):
                validator = StringContainsValidator()
                validator.setRequiredStrings(["Home","meOw"])
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = StringContainsValidatorWithMultipleItem()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "NotValid")
        self.assertRaises(ValueError, alg.setProperty, "Input", "HomeRenter")
        self.assertRaises(ValueError, alg.setProperty, "Input", "catmeOw")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", "HomeOwner")
        
    def test_StringContainsValidator_with_multiple_required_strings_constructor(self):
        """
            Test that a string-contains validator made from constructor that 
            supplies multiple strings required to all be contained in the input string
        """

        class StringContainsValidatorWithMultipleItem(PythonAlgorithm):

            def PyInit(self):
                validator = StringContainsValidator(["Home","meOw"])
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = StringContainsValidatorWithMultipleItem()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "NotValid")
        self.assertRaises(ValueError, alg.setProperty, "Input", "HomeRenter")
        self.assertRaises(ValueError, alg.setProperty, "Input", "catmeOw")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", "HomeOwner")


if __name__ == '__main__':
    unittest.main()
