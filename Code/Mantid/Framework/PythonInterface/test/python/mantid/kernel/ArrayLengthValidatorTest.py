import unittest
import testhelpers

from mantid.kernel import FloatArrayLengthValidator, FloatArrayProperty
from mantid.api import PythonAlgorithm

class ArrayLengthValidatorTest(unittest.TestCase):

    def test_empty_constructor_has_no_lengths_set(self):
        validator = FloatArrayLengthValidator()
        self.assertFalse(validator.hasLength())
        self.assertFalse(validator.hasMinLength())
        self.assertFalse(validator.hasMaxLength())

    def test_fixed_length_constructor_return_hasLength(self):
        fixedlength = 9
        validator = FloatArrayLengthValidator(fixedlength)
        self.assertTrue(validator.hasLength())
        self.assertEquals(validator.getLength(), fixedlength)

    def test_range_length_constructor_returns_hasMinMax(self):
        lower = 4
        upper = 7
        validator = FloatArrayLengthValidator(lower, upper)
        self.assertTrue(validator.hasMinLength())
        self.assertTrue(validator.hasMaxLength())
        self.assertEquals(validator.getMinLength(), lower)
        self.assertEquals(validator.getMaxLength(), upper)

    def test_setFixedLength_alters_accepted_lenth(self):
        validator = FloatArrayLengthValidator()
        self.assertFalse(validator.hasLength())
        fixed = 5
        validator.setLength(fixed)
        self.assertTrue(validator.hasLength())
        self.assertEquals(validator.getLength(), fixed)
        validator.clearLength()
        self.assertFalse(validator.hasLength())

    def test_setMinMaxLength_alters_accepted_range(self):
        validator = FloatArrayLengthValidator()
        self.assertFalse(validator.hasMinLength())
        self.assertFalse(validator.hasMaxLength())
        lower = 4
        upper = 7
        validator.setLengthMin(lower)
        self.assertTrue(validator.hasMinLength())
        self.assertFalse(validator.hasMaxLength())
        self.assertEquals(validator.getMinLength(), lower)
        validator.setLengthMax(upper)
        self.assertTrue(validator.hasMinLength())
        self.assertTrue(validator.hasMaxLength())
        self.assertEquals(validator.getMaxLength(), upper)

        validator.clearLengthMin()
        self.assertFalse(validator.hasMinLength())
        self.assertTrue(validator.hasMaxLength())
        validator.clearLengthMax()
        self.assertFalse(validator.hasMinLength())
        self.assertFalse(validator.hasMaxLength())

    def test_Validator_on_ArrayProperty_accepts_array_of_specified_length(self):
        fixedlength = 6
        alg = self._create_alg_with_fixedlength_validator(fixedlength)
        input_vals = [1.,2.4,5.6,8.0,4.6,6.]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input_vals)

    def test_Validator_on_ArrayProperty_rejects_array_of_without_correct_length(self):
        fixedlength = 6
        alg = self._create_alg_with_fixedlength_validator(fixedlength)
        input_vals = [1.,2.4,5.6]
        self.assertRaises(ValueError, alg.setProperty, "Input", input_vals)

    def test_Validator_on_ArrayProperty_accepts_array_with_length_in_range(self):
        alg = self._create_alg_with_range_validator(3,5)
        input_vals = []
        for i in range(1,7):
            input_vals.append(float(1))
            if i < 3 or i > 5:
                self.assertRaises(ValueError, alg.setProperty, "Input", input_vals)
            else:
                testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input_vals)

    def _create_alg_with_fixedlength_validator(self, fixedlength):
        """
            Creates a test algorithm with a fixed length validator
        """
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                validator = FloatArrayLengthValidator(fixedlength)
                self.declareProperty(FloatArrayProperty("Input", validator))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg

    def _create_alg_with_range_validator(self, minlength, maxlength):
        """
            Creates a test algorithm with a range length validator
        """
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                validator = FloatArrayLengthValidator(minlength, maxlength)
                self.declareProperty(FloatArrayProperty("Input", validator))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg


if __name__ == '__main__':
    unittest.main()
