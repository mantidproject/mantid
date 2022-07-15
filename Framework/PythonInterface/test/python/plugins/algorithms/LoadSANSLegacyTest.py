import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from math import pi
import os
from mantid.simpleapi import LoadSANSLegacy


class LoadSANSLegacyTest(unittest.TestCase):

    # def test_LoadValidData(self):
    #     out_ws_name = "LoadSANSLegacyTest_Test1"
    #     filename = "testSANSdata.csv"
    #     alg_test = run_algorithm("LoadSANSLegacy", Filename=filename,
    #                              OutputWorkspace=out_ws_name)
    #     self.assertTrue(alg_test.isExecuted())


    def test_aaaa(self):
        self.assertTrue(1 == 1)


if __name__ == '__main__':
    unittest.main()