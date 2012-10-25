import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from testhelpers import run_algorithm

# The only reason we have this test is because we currently can't remove python unit tests once they've gone in!
class Stitch1DTest(unittest.TestCase):

    def test_nothing(self):
        self.assertTrue(True)

        
if __name__ == '__main__':
    unittest.main()