import unittest
import testhelpers
from mantid import FloatBoundedValidator, IntBoundedValidator

class BoundedValidatorTest(object):

    def test_construction_does_not_raise_error_when_both_are_floats(self):
        testhelpers.assertRaisesNothing(self, FloatBoundedValidator, 1.0, 2.0)

    def test_construction_does_not_raise_error_when_both_are_ints(self):
        testhelpers.assertRaisesNothing(self, IntBoundedValidator, 1, 20)

    def test_construction_with_lower_sets_only_lower(self):
        validator = FloatBoundedValidator()
        lower = 1.4
        validator.setLower(lower)
        self.assertEquals(validator.hasLower(), True)
        self.assertEquals(validator.hasUpper(), False)
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
