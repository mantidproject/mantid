import unittest
import testhelpers
from mantid import BoundedValidator

class BoundedValidatorTest(object):

    def test_construction_does_not_raise_error_when_both_are_floats(self):
        testhelpers.assert_raises_nothing(self, BoundedValidator, 1.0, 2.0)

    def test_construction_does_not_raise_error_when_both_are_ints(self):
        testhelpers.assert_raises_nothing(self, BoundedValidator, 1, 20)

    def test_construction_raises_error_when_called_with_no_params(self):
        self.assertRaises(TypeError, BoundedValidator())

    def test_construction_with_lower_sets_only_lower(self):
        validator = BoundedValidator(lower=1)
        self.assertEquals(validator.hasLower(), True)
        self.assertEquals(validator.hasUpper(), False)
        self.assertEquals(validator.lower(), 1)

    def test_construction_with_upper_sets_only_upper(self):
        validator = BoundedValidator(upper=5.0)
        self.assertEquals(validator.hasUpper(), True)
        self.assertEquals(validator.hasLower(), False)
        self.assertEquals(validator.upper(), 5.0)


if __name__ == '__main__':
    unittest.main()
