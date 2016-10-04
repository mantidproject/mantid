from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import run_algorithm
from mantid.simpleapi import *


class DGSReductionILLTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        for ws in mtd.getObjectNames():
            if mtd.doesExist(ws):
                DeleteWorkspace(Workspace = ws)

    def load(self, filename, detectorList):
        Load(Filename=filename, OutputWorkspace='inputWS')
        ExtractSpectra(InputWorkspace='inputWS', OutputWorkspace ='smallWS', DetectorList = detectorList)

    def do_test_minimal(self):
        alg_test = run_algorithm("DGSReductionILL", InputWorkspace='smallWS', OutputWorkspace='outWS')

        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(mtd.doesExist('outWS'))

    def do_test_output_workspace_names(self):
        alg_test = run_algorithm("DGSReductionILL", InputWorkspace='smallWS', OutputWorkspace='outWS', OutputPrefix = 'test', ControlMode = True)

        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(mtd.doesExist('test_bkg'))
        self.assertTrue(mtd.doesExist('test_bkgsubtr'))
        self.assertTrue(mtd.doesExist('test_deteff'))
        self.assertTrue(mtd.doesExist('test_econv'))
        self.assertTrue(mtd.doesExist('test_epp'))
        self.assertTrue(mtd.doesExist('test_kikf'))
        self.assertTrue(mtd.doesExist('test_mask'))
        self.assertTrue(mtd.doesExist('test_masked'))
        self.assertTrue(mtd.doesExist('test_monepp'))
        self.assertTrue(mtd.doesExist('test_norm'))
        #self.assertTrue(mtd.doesExist('test_raw'))
        self.assertTrue(mtd.doesExist('EPPfit_NormalisedCovarianceMatrix'))
        self.assertTrue(mtd.doesExist('EPPfit_Parameters'))
        self.assertTrue(mtd.doesExist('monitorWorkspace'))
        self.assertTrue(mtd.doesExist('normFactor'))
        self.assertTrue(mtd.doesExist('workspace'))

    def test_in6_minimal(self):
        self.load('ILL/IN6/164192.nxs', '1,2,3,1001')
        self.do_test_minimal()

    def test_in4_minimal(self):
        self.load('ILL/IN4/084446.nxs', '0,1,2,3')
        self.do_test_minimal()

    def test_in6_output_workspace_names(self):
        self.load('ILL/IN6/164192.nxs', '1,2,3,1001')
        self.do_test_output_workspace_names()

    def test_in4_output_workspace_names(self):
        self.load('ILL/IN4/084446.nxs', '0,1,2,3')
        self.do_test_output_workspace_names()

if __name__ == '__main__':
    unittest.main()
