from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import run_algorithm
from mantid.simpleapi import *


class DGSReductionILLTest(unittest.TestCase):

    _output_workspaces = []

    def setUp(self):
        self._input_file = 'ILL/IN6/164192.nxs'

    def tearDown(self):
        pass

    def test_minimal(self):
        Load(Filename=self._input_file, OutputWorkspace='outWS')
        ExtractSpectra(InputWorkspace='outWS', OutputWorkspace ='outWS2', DetectorList = '1,2,3,1001')
        alg_test = run_algorithm("DGSReductionILL", InputWorkspace='outWS2', OutputWorkspace='outWS')

        self.assertTrue(alg_test.isExecuted())

if __name__ == '__main__':
    unittest.main()
