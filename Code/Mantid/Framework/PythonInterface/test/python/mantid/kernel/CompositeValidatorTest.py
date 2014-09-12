import unittest
from mantid.kernel import CompositeValidator, FloatBoundedValidator
from mantid.api import PythonAlgorithm

class CompositeValidatorTest(unittest.TestCase):

    def test_creation_with_add_succeeds_correctly_in_algorithm(self):
        """
            Tests that a composite validator created with the add
            method validates correctly
        """
        validation = CompositeValidator()
        validation.add(FloatBoundedValidator(lower=5))
        validation.add(FloatBoundedValidator(upper=10))
        self._do_validation_test(validation)

    def test_creation_with_constructor_and_list(self):
        """
            Tests that a composite validator created with the constructor method
        """
        validation = CompositeValidator([FloatBoundedValidator(lower=5), FloatBoundedValidator(upper=10)])
        self._do_validation_test(validation)

    def _do_validation_test(self, validation):
        """Run the validator tests"""
        test_alg = self._create_test_algorithm(validation)
        prop = test_alg.getProperty("Input")
        self.assertNotEquals(prop.isValid, "")
        test_alg.setProperty("Input", 6.8)
        self.assertEquals(prop.isValid, "")
        self.assertRaises(ValueError, test_alg.setProperty, "Input", 15)

    def _create_test_algorithm(self, validator):
        """Create a test algorithm"""
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                self.declareProperty("Input", -1.0, validator)

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg


if __name__ == '__main__':
    unittest.main()
