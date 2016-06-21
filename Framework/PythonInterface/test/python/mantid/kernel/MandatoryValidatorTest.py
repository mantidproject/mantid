import unittest
import testhelpers
from mantid.kernel import FloatArrayProperty, StringMandatoryValidator, FloatArrayMandatoryValidator
from mantid.api import PythonAlgorithm

class MandatoryValidatorTest(unittest.TestCase):

    def test_constructor_does_not_raise_error(self):
        testhelpers.assertRaisesNothing(self, StringMandatoryValidator)

    def test_validator_restricts_property_values_to_non_empty(self):
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                self.declareProperty("StringInput", "", StringMandatoryValidator())
                self.declareProperty(FloatArrayProperty("ArrayInput", FloatArrayMandatoryValidator()))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()

        self.assertRaises(ValueError, alg.setProperty, "StringInput", "")
        testhelpers.assertRaisesNothing(self, alg.setProperty, "StringInput", "value")

        self.assertRaises(ValueError, alg.setProperty, "ArrayInput", [])
        testhelpers.assertRaisesNothing(self, alg.setProperty, "ArrayInput", [1.2,3.4])

if __name__ == '__main__':
    unittest.main()
