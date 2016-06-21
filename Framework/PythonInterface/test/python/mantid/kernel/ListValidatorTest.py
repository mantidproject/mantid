import unittest
import testhelpers

from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm

class ListValidatorTest(unittest.TestCase):

    def test_empty_ListValidator_allows_nothing(self):
        """
            Test that a list validator restricts the values
            for a property
        """

        class EmptyListValidator(PythonAlgorithm):

            def PyInit(self):
                validator = StringListValidator()
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = EmptyListValidator()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "AnyOldString")

    def test_ListValidator_plus_addAllowedValued_allows_that_value(self):
        """
            Test that a list validator restricts the values
            for a property
        """

        class SingleItemListValidator(PythonAlgorithm):

            _allowed = "OnlyThis"

            def PyInit(self):
                validator = StringListValidator()
                validator.addAllowedValue(self._allowed)
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = SingleItemListValidator()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "NotValid")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", alg._allowed)

    def test_ListValidator_with_values_in_constructor_restricts_property_values(self):
        """
            Test that a list validator restricts the values
            for a property
        """

        class MultiValueValidator(PythonAlgorithm):

            _allowed_vals = ["Val1", "Val2","Val3"]

            def PyInit(self):
                validator = StringListValidator(self._allowed_vals)
                self.declareProperty("Input", "", validator)

            def PyExec(self):
                pass

        alg = MultiValueValidator()
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Input", "NotValid")
        for val in alg._allowed_vals:
            testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", val)


if __name__ == '__main__':
    unittest.main()
