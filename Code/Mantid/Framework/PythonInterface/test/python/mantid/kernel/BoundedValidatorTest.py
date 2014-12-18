import unittest
import testhelpers
from mantid.kernel import FloatBoundedValidator, IntBoundedValidator

class BoundedValidatorTest(unittest.TestCase):

    def test_construction_does_not_raise_error_when_both_are_floats(self):
        testhelpers.assertRaisesNothing(self, FloatBoundedValidator, 1.0, 2.0)

    def test_construction_with_Exclusive_bounds_with_floats(self):
        testhelpers.assertRaisesNothing(self, FloatBoundedValidator, 1.0, 2.0, True)

    def test_constructor_sets_both_boundary_values_correctly(self):
        validator = FloatBoundedValidator(1.3, 2.6)
        self.assertTrue(validator.hasLower())
        self.assertEquals(validator.lower(), 1.3)
        self.assertTrue(validator.hasUpper())
        self.assertEquals(validator.upper(), 2.6)
        self.assertFalse(validator.isLowerExclusive())
        self.assertFalse(validator.isUpperExclusive())

    def test_constructor_sets_both_Exclusive_boundary_values_correctly(self):
        validator = FloatBoundedValidator(1.3, 2.6, True)
        self.assertTrue(validator.hasLower())
        self.assertEquals(validator.lower(), 1.3)
        self.assertTrue(validator.hasUpper())
        self.assertEquals(validator.upper(), 2.6)
        self.assertTrue(validator.isLowerExclusive())
        self.assertTrue(validator.isUpperExclusive())

    def test_construction_does_not_raise_error_when_both_are_ints(self):
        testhelpers.assertRaisesNothing(self, IntBoundedValidator, 1, 20)

    def test_construction_with_Exclusive_bounds_with_ints(self):
        testhelpers.assertRaisesNothing(self, IntBoundedValidator, 1, 20, True)

    def test_lower_only_keyword_in_constructor(self):
        validator = FloatBoundedValidator(lower=2.5)
        self.assertTrue(validator.hasLower())
        self.assertEquals(validator.lower(), 2.5)
        self.assertFalse(validator.hasUpper())

    def test_upper_only_keyword_in_constructor(self):
        validator = FloatBoundedValidator(upper=5.5)
        self.assertFalse(validator.hasLower())
        self.assertTrue(validator.hasUpper())
        self.assertEquals(validator.upper(), 5.5)

    def test_construction_with_lower_sets_only_lower(self):
        validator = FloatBoundedValidator()
        lower = 1.4
        validator.setLower(lower)
        self.assertEquals(validator.hasLower(), True)
        self.assertFalse(validator.hasUpper())
        self.assertEquals(validator.lower(), lower)

    def test_construction_with_upper_sets_only_upper(self):
        validator = FloatBoundedValidator()
        upper = 5.4
        validator.setUpper(upper)
        self.assertEquals(validator.hasUpper(), True)
        self.assertEquals(validator.hasLower(), False)
        self.assertEquals(validator.upper(), upper)

if __name__ == '__main__':
    unittest.main()
