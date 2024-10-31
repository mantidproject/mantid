# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateSampleWorkspace, CloneWorkspace, GroupWorkspaces, AddSampleLogMultiple, CreateEmptyTableWorkspace
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService, WorkspaceGroup
from scipy.constants import h, m_n, eV
import numpy as np


class CorrectTOFTest(unittest.TestCase):
    def setUp(self):
        # create sample workspace
        self.xmin = 2123.33867005 + 4005.75
        self.xmax = 2123.33867005 + 7995.75

        self._input_ws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=LinearBackground, \
                                               A0=0.3;name=Gaussian, PeakCentre=8190, Height=5, Sigma=75",
            NumBanks=2,
            BankPixelWidth=1,
            XMin=self.xmin,
            XMax=self.xmax,
            BinWidth=10.5,
            BankDistanceFromSample=4.0,
            SourceDistanceFromSample=1.4,
            OutputWorkspace="ws",
        )
        lognames = "wavelength,TOF1"
        logvalues = "6.0,2123.33867005"
        AddSampleLogMultiple(self._input_ws, lognames, logvalues)
        # create EPP table
        self._table = CreateEmptyTableWorkspace(OutputWorkspace="epptable")
        self._table.addColumn(type="double", name="PeakCentre")
        table_row = {"PeakCentre": 8189.5}
        for i in range(2):
            self._table.addRow(table_row)

    def tearDown(self):
        for wsname in ["ws", "epptable"]:
            if AnalysisDataService.doesExist(wsname):
                run_algorithm("DeleteWorkspace", Workspace=wsname)

    def testCorrection(self):
        # tests that correction is done properly
        OutputWorkspaceName = "outputws1"
        alg_test = run_algorithm("CorrectTOF", InputWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        velocity = h / (m_n * 6.0e-10)
        t_el = 4.0e6 / velocity
        t_corr = np.arange(self.xmin, self.xmax + 1.0, 10.5) + t_el - (8189.5 - 2123.33867005)
        self.assertTrue(np.allclose(t_corr, wsoutput.readX(0)))  # sdd = 4
        self.assertTrue(np.allclose(t_corr + t_el, wsoutput.readX(1)))  # sdd = 8

        run_algorithm("DeleteWorkspace", Workspace=wsoutput)

    def testGroup(self):
        # tests whether the group of workspaces is accepted as an input
        ws2 = CloneWorkspace(self._input_ws)
        group = GroupWorkspaces([self._input_ws, ws2])
        OutputWorkspaceName = "output_wsgroup"
        alg_test = run_algorithm("CorrectTOF", InputWorkspace="group", EPPTable=self._table, OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        self.assertTrue(isinstance(wsoutput, WorkspaceGroup))
        self.assertEqual(2, wsoutput.getNumberOfEntries())

        run_algorithm("DeleteWorkspace", Workspace=group)
        run_algorithm("DeleteWorkspace", Workspace=wsoutput)

    def testConvertUnits(self):
        # test whether CorrectTof+ConvertUnits+ConvertToDistribution will give the same result as TOFTOFConvertTOFToDeltaE
        OutputWorkspaceName = "outputws1"
        alg_test = run_algorithm("CorrectTOF", InputWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())
        wscorr = AnalysisDataService.retrieve(OutputWorkspaceName)

        # convert units, convert to distribution
        run_algorithm(
            "ConvertUnits", InputWorkspace=wscorr, Target="DeltaE", EMode="Direct", EFixed=2.27, OutputWorkspace=OutputWorkspaceName + "_dE"
        )
        ws_dE = AnalysisDataService.retrieve(OutputWorkspaceName + "_dE")
        run_algorithm("ConvertToDistribution", Workspace=ws_dE)

        # create reference data for X axis
        tof1 = 2123.33867005
        dataX = self._input_ws.readX(0) - tof1
        tel = 8189.5 - tof1
        factor = m_n * 1e15 / eV
        newX = 0.5 * factor * 16.0 * (1 / tel**2 - 1 / dataX**2)
        # compare
        # self.assertEqual(newX[0], ws_dE.readX(0)[0])
        self.assertTrue(np.allclose(newX, ws_dE.readX(0), atol=0.01))

        # create reference data for Y axis and compare to the output
        tof = dataX[:-1] + 5.25
        newY = self._input_ws.readY(0) * tof**3 / (factor * 10.5 * 16.0)
        # compare
        self.assertTrue(np.allclose(newY, ws_dE.readY(0), rtol=0.01))

        run_algorithm("DeleteWorkspace", Workspace=ws_dE)
        run_algorithm("DeleteWorkspace", Workspace=wscorr)


if __name__ == "__main__":
    unittest.main()
