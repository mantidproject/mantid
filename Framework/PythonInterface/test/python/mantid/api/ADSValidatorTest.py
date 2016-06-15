import unittest
import testhelpers

from mantid.api import PythonAlgorithm, ADSValidator

class ADSValidatorTest(unittest.TestCase):

    def test_empty_constructor(self):
        validator = ADSValidator()
        self.assertTrue(validator.isMultiSelectionAllowed())
        self.assertFalse(validator.isOptional())

    def test__constructor(self):
        validator = ADSValidator(False, True)
        self.assertFalse(validator.isMultiSelectionAllowed())
        self.assertTrue(validator.isOptional())

if __name__ == '__main__':
    unittest.main()
