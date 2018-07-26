# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import unittest
import os
import stresstesting

import mantid
from mantid.api import AlgorithmManager

from mantid.simpleapi import BiblySANSDATAProcessor

# -----------------------------------------------
# Tests for the BilbySANSDataProcessorTest algorithm
# -----------------------------------------------
class BilbySANSDataProcessorTest(unittest.TestCase):
    def test_this_is_an_example_test(self):
        # Given : This sets up the inputs to the algorithm call

        # When : Here we make the algorithm call
        output_workspace, transmission_fit = BilbySANSDataProcessor(**args, **kwargs)

        # Then : Here we test the outputs are correct
        self.assertTrue(False)



class BilbySANSDataProcessorTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(BilbySANSDataProcessorTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success


if __name__ == '__main__':
    unittest.main()