import unittest
import testhelpers

from mantid.kernel import FloatArrayBoundedValidator, FloatArrayProperty
from mantid.api import PythonAlgorithm

class ArrayBoundedValidatorTest(unittest.TestCase):

    def test_empty_constructor_gives_no_lower_upper_bound_set(self):
        validator = FloatArrayBoundedValidator()
        self.assertFalse(validator.hasLower())
        self.assertFalse(validator.hasUpper())

    def test_set_members_alter_bounds(self):
        validator = FloatArrayBoundedValidator()
        self.assertFalse(validator.hasLower())
        self.assertFalse(validator.hasUpper())
        lower = 5.6
        validator.setLower(lower)
        self.assertTrue(validator.hasLower())
        self.assertEquals(validator.lower(), lower)
        self.assertFalse(validator.hasUpper())
        upper = 10.6
        validator.setUpper(upper)
        self.assertTrue(validator.hasLower())
        self.assertTrue(validator.hasUpper())
        self.assertEquals(validator.upper(), upper)

    def test_clear_members_remove_bounds(self):
        lower = 7.0
        upper = 10.0
        validator = FloatArrayBoundedValidator(lower, upper)
        self.assertTrue(validator.hasLower())
        self.assertTrue(validator.hasUpper())
        self.assertEquals(validator.lower(), lower)
        self.assertEquals(validator.upper(), upper)
        validator.clearLower()
        self.assertFalse(validator.hasLower())
        self.assertTrue(validator.hasUpper())
        validator.clearUpper()
        self.assertFalse(validator.hasLower())
        self.assertFalse(validator.hasUpper())


    def test_values_within_array_bounds_are_accepted_by_validator(self):
       alg = self._create_alg_with_ArrayBoundedValidator(5.1, 10.4)
       input_vals = [5.1, 5.6, 10.4, 9.2]
       testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input_vals)

    def test_values_lower_than_array_bounds_are_not_accepted_by_validator(self):
       alg = self._create_alg_with_ArrayBoundedValidator(5.1, 10.4)
       input_vals = [5.4, 6.2, 1.3]
       self.assertRaises(ValueError, alg.setProperty, "Input", input_vals)

    def test_values_greater_than_array_bounds_are_not_accepted_by_validator(self):
       alg = self._create_alg_with_ArrayBoundedValidator(5.1, 10.4)
       input_vals = [5.4, 20.1, 8.3, ]
       self.assertRaises(ValueError, alg.setProperty, "Input", input_vals)

    def _create_alg_with_ArrayBoundedValidator(self, lower, upper):
        """
            Creates a test algorithm with a bounded validator
        """
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                validator = FloatArrayBoundedValidator(lower, upper)
                self.declareProperty(FloatArrayProperty("Input", validator))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg

if __name__ == '__main__':
    unittest.main()
