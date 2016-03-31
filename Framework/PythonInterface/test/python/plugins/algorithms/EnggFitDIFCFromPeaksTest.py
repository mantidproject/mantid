import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnggFitDIFCFromPeaksTest(unittest.TestCase):

    def test_wrong_properties(self):
        """
        Handle in/out property issues appropriately.
        """
        pass


if __name__ == '__main__':
    unittest.main()
