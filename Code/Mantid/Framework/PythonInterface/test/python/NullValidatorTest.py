import unittest
import testhelpers
from mantid import NullValidator

class NullValidatorTest(unittest.TestCase):

    def test_NullValidator_can_be_default_constructed(self):
        testhelpers.assertRaisesNothing(self, NullValidator)

if __name__ == '__main__':
    unittest.main()
