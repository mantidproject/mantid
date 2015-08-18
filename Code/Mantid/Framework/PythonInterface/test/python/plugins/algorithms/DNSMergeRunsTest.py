import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSMergeRuns


class DNSMergeRunsTest(unittest.TestCase):
    workspaces = []
    angles = None

    def _create_fake_workspace(self, wsname, angle):
        """
        creates DNS workspace with fake data
        """
        ndet = 24
        dataX = np.zeros(2*ndet)
        dataX.fill(4.2 + 0.00001)
        dataX[::2] -= 0.000002
        dataY = np.ones(ndet)
        dataE = np.sqrt(dataY)
        # create workspace
        api.CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        outws = api.mtd[wsname]
        api.LoadInstrument(outws, InstrumentName='DNS')
        p_names = 'wavelength,slit_i_left_blade_position,slit_i_right_blade_position,normalized,\
            slit_i_lower_blade_position,slit_i_upper_blade_position,polarisation,flipper'
        p_values = '4.2,10,10,no,5,20,x,ON'
        api.AddSampleLogMultiple(Workspace=outws, LogNames=p_names, LogValues=p_values, ParseType=True)
        # rotate instrument component ans set deterota
        api.RotateInstrumentComponent(outws, "bank0", X=0, Y=1, Z=0, Angle=angle)
        api.AddSampleLog(outws, LogName='deterota', LogText=str(angle),
                         LogType='Number', LogUnit='Degrees')
        # create the normalization workspace
        dataY.fill(1.0)
        dataE.fill(1.0)
        api.CreateWorkspace(OutputWorkspace=wsname + '_NORM', DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        normws = api.mtd[wsname + '_NORM']
        api.LoadInstrument(normws, InstrumentName='DNS')
        api.AddSampleLogMultiple(Workspace=normws, LogNames=p_names, LogValues=p_values, ParseType=True)

        return outws

    def setUp(self):
        self.workspaces = []
        for i in range(4):
            angle = -7.5 - i*0.5
            wsname = "__testws" + str(i)
            self._create_fake_workspace(wsname, angle)
            self.workspaces.append(wsname)
        # create reference values
        angles = np.empty(0)
        for j in range(4):
            a = 7.5 + j*0.5
            angles = np.hstack([angles, np.array([i*5 + a for i in range(24)])])
        self.angles = np.sort(angles)

    def tearDown(self):
        for wsname in self.workspaces:
            if api.AnalysisDataService.doesExist(wsname + '_NORM'):
                api.DeleteWorkspace(wsname + '_NORM')
            if api.AnalysisDataService.doesExist(wsname):
                api.DeleteWorkspace(wsname)
        self.workspaces = []

    def test_DNSNormWorkspaceExists(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test1"
        api.DeleteWorkspace(self.workspaces[0] + '_NORM')
        self.assertRaises(RuntimeError, DNSMergeRuns, WorkspaceNames=self.workspaces,
                          OutputWorkspace=outputWorkspaceName)
        return

    def test_DNSSameWavelength(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test2"
        ws = api.AnalysisDataService.retrieve(self.workspaces[0])
        api.AddSampleLog(ws, LogName='wavelength', LogText=str(5.0),
                         LogType='Number', LogUnit='Angstrom')
        self.assertRaises(RuntimeError, DNSMergeRuns, WorkspaceNames=self.workspaces,
                          OutputWorkspace=outputWorkspaceName)
        return

    def test_DNSTwoTheta(self):
        outputWorkspaceName = "DNSMergeRunsTest_Test3"
        alg_test = run_algorithm("DNSMergeRuns", WorkspaceNames=self.workspaces,
                                 OutputWorkspace=outputWorkspaceName,  HorizontalAxis='2theta')

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2,  ws.getNumDims())
        self.assertEqual(1,  ws.getNumberHistograms())
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
        alg_test = run_algorithm("DNSMergeRuns", WorkspaceNames=self.workspaces,
                                 OutputWorkspace=outputWorkspaceName, HorizontalAxis='|Q|')

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2,  ws.getNumDims())
        self.assertEqual(1,  ws.getNumberHistograms())
        # data array
        # reference values
        ttheta = np.round(np.radians(self.angles), 4)
        qarr = np.sort(4.0*np.pi*np.sin(0.5*ttheta)/4.2)
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
        alg_test = run_algorithm("DNSMergeRuns", WorkspaceNames=self.workspaces,
                                 OutputWorkspace=outputWorkspaceName, HorizontalAxis='d-Spacing')

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(96, ws.blocksize())
        self.assertEqual(2,  ws.getNumDims())
        self.assertEqual(1,  ws.getNumberHistograms())
        # data array
        # reference values
        ttheta = np.round(np.radians(self.angles), 4)
        darr = np.sort(4.2*0.5/np.sin(0.5*ttheta))
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

if __name__ == '__main__':
    unittest.main()
