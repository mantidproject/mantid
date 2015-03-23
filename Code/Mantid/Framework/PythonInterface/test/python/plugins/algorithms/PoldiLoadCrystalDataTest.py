import unittest

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class PoldiLoadCrystalDataTest(unittest.TestCase):
    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_Init(self):
        self.assertTrue(False)


if __name__ == '__main__':
    unittest.main()