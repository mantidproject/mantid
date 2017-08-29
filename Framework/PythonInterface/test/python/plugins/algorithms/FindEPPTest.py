from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import DeleteWorkspace, CreateSampleWorkspace, CloneWorkspace, GroupWorkspaces
from mantid.api import AnalysisDataService, WorkspaceGroup, mtd
import numpy as np
from testhelpers import run_algorithm


class FindEPPTest(unittest.TestCase):

    def setUp(self):
        # create sample workspace
        self._input_ws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
                                               A0=0.3;name=Gaussian, PeakCentre=6000, Height=5, Sigma=75", NumBanks=2,
                                               BankPixelWidth=1, XMin=4005.75, XMax=7995.75, BinWidth=10.5,
                                               BankDistanceFromSample=4.0, OutputWorkspace="ws")

    def tearDown(self):
        if AnalysisDataService.doesExist('ws'):
            DeleteWorkspace(self._input_ws)

    def testTable(self):
        # tests that correct table is created
        OutputWorkspaceName = "outputws1"
        alg_test = run_algorithm("FindEPP", InputWorkspace=self._input_ws, OutputWorkspace=OutputWorkspaceName, Version=1)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertEqual(2, wsoutput.rowCount())
        self.assertEqual(9, wsoutput.columnCount())
        columns = ['WorkspaceIndex', 'PeakCentre', 'PeakCentreError', 'Sigma', 'SigmaError', 'Height', 'HeightError', 'chiSq', 'FitStatus']
        self.assertEqual(columns, wsoutput.getColumnNames())
        DeleteWorkspace(wsoutput)

    def testGroup(self):
        # tests whether the group of workspaces is accepted as an input
        ws2 = CloneWorkspace(self._input_ws)
        group = GroupWorkspaces([self._input_ws, ws2])
        OutputWorkspaceName = "output_wsgroup"
        alg_test = run_algorithm("FindEPP", InputWorkspace='group', OutputWorkspace=OutputWorkspaceName, Version=1)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertTrue(isinstance(wsoutput, WorkspaceGroup))
        self.assertEqual(2, wsoutput.getNumberOfEntries())

        run_algorithm("DeleteWorkspace", Workspace=group)
        run_algorithm("DeleteWorkspace", Workspace=wsoutput)

    def testFitSuccess(self):
        # tests successful fit
        OutputWorkspaceName = "outputws2"
        alg_test = run_algorithm("FindEPP", InputWorkspace=self._input_ws, OutputWorkspace=OutputWorkspaceName, Version=1)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertEqual(['success', 'success'], wsoutput.column(8))
        self.assertEqual([0, 1], wsoutput.column(0))
        self.assertAlmostEqual(6005.25, wsoutput.column(1)[0], places=3)
        self.assertAlmostEqual(6005.25, wsoutput.column(1)[1], places=3)
        self.assertAlmostEqual(89.3249, wsoutput.column(3)[0], places=4)
        self.assertAlmostEqual(89.3249, wsoutput.column(3)[1], places=4)
        run_algorithm("DeleteWorkspace", Workspace=wsoutput)

    def _float_values_for_row(self, wks, idx):
        """ gets in a list the float values in a row of a table workspace """
        return [val for key, val in wks.row(idx).items() if isinstance(val, float)]

    def testFitFailedLinear(self):
        # tests failed fit
        ws_linear = CreateSampleWorkspace(Function="User Defined",UserDefinedFunction="name=LinearBackground,A0=0.0;",
                                          NumBanks=2, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

        OutputWorkspaceName = "outputws3"
        alg_test = run_algorithm("FindEPP", InputWorkspace=ws_linear, OutputWorkspace=OutputWorkspaceName, Version=1)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertEqual(['failed', 'failed'], wsoutput.column(8))
        self.assertEqual([0, 1], wsoutput.column(0))
        zeros = np.zeros(7)
        self.assertTrue(np.allclose(zeros, self._float_values_for_row(wsoutput, 0)))
        self.assertTrue(np.allclose(zeros, self._float_values_for_row(wsoutput, 1)))

        run_algorithm("DeleteWorkspace", Workspace=wsoutput)
        run_algorithm("DeleteWorkspace", Workspace=ws_linear)

    def testFitFailedNarrow(self):
        # tests failed fit
        ws_narrow = CreateSampleWorkspace(Function="User Defined",UserDefinedFunction="name=Gaussian,PeakCentre=5,Height=1,Sigma=0.05",
                                          NumBanks=2, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

        OutputWorkspaceName = "outputws4"
        alg_test = run_algorithm("FindEPP", InputWorkspace=ws_narrow, OutputWorkspace=OutputWorkspaceName, Version=1)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertEqual(['failed', 'failed'], wsoutput.column(8))
        self.assertEqual([0, 1], wsoutput.column(0))
        zeros = np.zeros(7)
        self.assertTrue(np.allclose(zeros, self._float_values_for_row(wsoutput, 0)))
        self.assertTrue(np.allclose(zeros, self._float_values_for_row(wsoutput, 1)))

        run_algorithm("DeleteWorkspace", Workspace=wsoutput)
        run_algorithm("DeleteWorkspace", Workspace=ws_narrow)

    def testFitOutputWorkspacesAreDeleted(self):
        OutputWorkspaceName = "outputws1"
        alg_test = run_algorithm("FindEPP", InputWorkspace=self._input_ws, OutputWorkspace=OutputWorkspaceName, Version=1)
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        DeleteWorkspace(wsoutput)
        oldOption = mantid.config['MantidOptions.InvisibleWorkspaces']
        mantid.config['MantidOptions.InvisibleWorkspaces'] = '1'
        self.assertEqual(mtd.size(), 1) # Only self._input_ws exists.
        mantid.config['MantidOptions.InvisibleWorkspaces'] = oldOption

if __name__ == "__main__":
    unittest.main()
