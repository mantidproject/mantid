# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import run_algorithm
from testhelpers.mlzhelpers import create_fake_dns_workspace
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSMergeRuns


class DNSMergeRunsTest(unittest.TestCase):
    workspaces = []
    angles = None

    def setUp(self):
        self.workspaces = []
        for i in range(4):
            angle = -7.5 - i * 0.5
            wsname = "__testws" + str(i)
            create_fake_dns_workspace(wsname, angle=angle, loadinstrument=True)
            self.workspaces.append(wsname)
        # create reference values
        angles = np.empty(0)
        for j in range(4):
            a = 7.5 + j * 0.5
            angles = np.hstack([angles, np.array([i * 5 + a for i in range(24)])])
        self.angles = np.sort(angles)

    def tearDown(self):
        for wsname in self.workspaces:
            if api.AnalysisDataService.doesExist(wsname):
                api.DeleteWorkspace(wsname)
        self.workspaces = []

    def test_DNSSameWavelength(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test1"
        ws = api.AnalysisDataService.retrieve(self.workspaces[0])
        api.AddSampleLog(ws, LogName="wavelength", LogText=str(5.0), LogType="Number", LogUnit="Angstrom")
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot merge workspaces with different wavelength",
            DNSMergeRuns,
            WorkspaceNames=self.workspaces,
            OutputWorkspace=outputWorkspaceName,
        )
        return

    def test_DNSSameNormalization(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test2"
        ws = api.AnalysisDataService.retrieve(self.workspaces[0])
        api.AddSampleLog(ws, LogName="normalized", LogText="no", LogType="String")
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot merge workspaces with different normalized",
            DNSMergeRuns,
            WorkspaceNames=self.workspaces,
            OutputWorkspace=outputWorkspaceName,
        )
        return

    def test_DNSTwoTheta(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test3"
        alg_test = run_algorithm(
            "DNSMergeRuns", WorkspaceNames=self.workspaces, OutputWorkspace=outputWorkspaceName, HorizontalAxis="2theta"
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2, ws.getNumDims())
        self.assertEqual(1, ws.getNumberHistograms())
        # data array
        # read the merged values
        dataX = ws.extractX()[0]
        for i in range(len(self.angles)):
            self.assertAlmostEqual(self.angles[i], dataX[i])
        # check that the intensity has not been changed
        dataY = ws.extractY()[0]
        for i in range(len(dataY)):
            self.assertAlmostEqual(1.0, dataY[i])
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_DNSMomentumTransfer(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test4"
        alg_test = run_algorithm("DNSMergeRuns", WorkspaceNames=self.workspaces, OutputWorkspace=outputWorkspaceName, HorizontalAxis="|Q|")

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2, ws.getNumDims())
        self.assertEqual(1, ws.getNumberHistograms())
        # data array
        # reference values
        ttheta = np.round(np.radians(self.angles), 4)
        qarr = np.sort(4.0 * np.pi * np.sin(0.5 * ttheta) / 4.2)
        # read the merged values
        dataX = ws.extractX()[0]
        for i in range(len(self.angles)):
            self.assertAlmostEqual(qarr[i], dataX[i])
        # check that the intensity has not been changed
        dataY = ws.extractY()[0]
        for i in range(len(dataY)):
            self.assertAlmostEqual(1.0, dataY[i])
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_DNSDSpacing(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test5"
        alg_test = run_algorithm(
            "DNSMergeRuns", WorkspaceNames=self.workspaces, OutputWorkspace=outputWorkspaceName, HorizontalAxis="d-Spacing"
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2, ws.getNumDims())
        self.assertEqual(1, ws.getNumberHistograms())
        # data array
        # reference values
        ttheta = np.round(np.radians(self.angles), 4)
        darr = np.sort(4.2 * 0.5 / np.sin(0.5 * ttheta))
        # read the merged values
        dataX = ws.extractX()[0]
        for i in range(len(self.angles)):
            self.assertAlmostEqual(darr[i], dataX[i])
        # check that the intensity has not been changed
        dataY = ws.extractY()[0]
        for i in range(len(dataY)):
            self.assertAlmostEqual(1.0, dataY[i])
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_DNSTwoTheta_Groups(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test3"
        api.GroupWorkspaces(self.workspaces, OutputWorkspace="group")
        alg_test = run_algorithm("DNSMergeRuns", WorkspaceNames="group", OutputWorkspace=outputWorkspaceName, HorizontalAxis="2theta")

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2, ws.getNumDims())
        self.assertEqual(1, ws.getNumberHistograms())
        # data array
        # read the merged values
        dataX = ws.extractX()[0]
        for i in range(len(self.angles)):
            self.assertAlmostEqual(self.angles[i], dataX[i])
        # check that the intensity has not been changed
        dataY = ws.extractY()[0]
        for i in range(len(dataY)):
            self.assertAlmostEqual(1.0, dataY[i])
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return


if __name__ == "__main__":
    unittest.main()
