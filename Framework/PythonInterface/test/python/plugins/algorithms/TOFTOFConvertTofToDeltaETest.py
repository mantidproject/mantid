import unittest
from mantid.simpleapi import DeleteWorkspace, CreateSampleWorkspace, AddSampleLogMultiple, AddSampleLog, \
    TOFTOFConvertTofToDeltaE, DeleteLog
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
from scipy.constants import m_n, eV


class TOFTOFConvertTofToDeltaETest(unittest.TestCase):
    def setUp(self):
        # create sample workspace
        ws_tof = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
                                       A0=0.3;name=Gaussian, PeakCentre=6000, Height=5, Sigma=75", NumBanks=2,
                                       BankPixelWidth=1, XMin=4005.75, XMax=7995.75, BinWidth=10.5,
                                       BankDistanceFromSample=4.0, SourceDistanceFromSample=8.0)
        # add required sample logs
        lognames = "channel_width,chopper_ratio,chopper_speed,Ei,wavelength,EPP"
        logvalues = "10.5,5,14000,2.27,6,190.0"
        AddSampleLogMultiple(ws_tof, lognames, logvalues)
        self._input_ws = ws_tof

    def test_basicrun(self):
        OutputWorkspaceName = "outputws"
        alg_test = run_algorithm("TOFTOFConvertTofToDeltaE", InputWorkspace=self._input_ws,
                                 OutputWorkspace=OutputWorkspaceName, ChoiceElasticTof="Geometry")
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        # execution of algorithm
        self.assertTrue(alg_test.isExecuted())
        # unit of output
        self.assertEqual(wsoutput.getAxis(0).getUnit().unitID(), "DeltaE")
        # shape of output compared to input
        self.assertEqual(wsoutput.getNumberHistograms(), self._input_ws.getNumberHistograms())
        self.assertEqual(wsoutput.blocksize(), self._input_ws.blocksize())
        DeleteWorkspace(wsoutput)

    def test_defaultsetup(self):
        OutputWorkspaceName = "outputws"
        alg_test = run_algorithm("TOFTOFConvertTofToDeltaE", InputWorkspace=self._input_ws,
                                 OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())
        self.assertEqual(alg_test.getProperty("ChoiceElasticTof").value, "Geometry")
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)
        DeleteWorkspace(wsoutput)

    # Test options to calculate tof elastic
    def test_calculation_geometry(self):
        OutputWorkspaceName = "outputws"
        alg_test = run_algorithm("TOFTOFConvertTofToDeltaE", InputWorkspace=self._input_ws,
                                 OutputWorkspace=OutputWorkspaceName, ChoiceElasticTof='Geometry')
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)

        # create reference data for X axis
        dataX = np.linspace(4005.75, 7995.75, 381)
        tel = 190.0*10.5 + 4005.75
        factor = m_n*1e+15/eV
        newX = 0.5*factor*12.0*12.0*(1/tel**2 - 1/dataX**2)
        # compare
        self.assertTrue(np.allclose(newX, wsoutput.readX(0)))           # sdd = 4.0, l1 =8.0
        self.assertTrue(np.allclose(16.0*newX/9.0, wsoutput.readX(1)))       # sdd = 8.0

        # create reference data for Y axis and compare to the output
        tof = dataX[:-1] + 5.25
        newY = self._input_ws.readY(0)*tof**3/(factor*10.5*12.0*12.0)
        self.assertTrue(np.allclose(newY, wsoutput.readY(0)))           # sdd = 4.0
        self.assertTrue(np.allclose(9.0*newY/16.0, wsoutput.readY(1)))       # sdd = 8.0

        DeleteWorkspace(wsoutput)

    def test_calculation_fitsample(self):
        OutputWorkspaceName = "outputws"
        alg_test = run_algorithm("TOFTOFConvertTofToDeltaE", InputWorkspace=self._input_ws,
                                 OutputWorkspace=OutputWorkspaceName, ChoiceElasticTof='FitSample')
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)

        # create reference data for X axis
        dataX = np.linspace(4005.75, 7995.75, 381)
        tel = 6005.25
        factor = m_n*1e+15/eV
        newX = 0.5*factor*12.0*12.0*(1/tel**2 - 1/dataX**2)
        # compare
        self.assertTrue(np.allclose(newX, wsoutput.readX(0)))           # sdd = 4.0
        self.assertTrue(np.allclose(16.0*newX/9.0, wsoutput.readX(1)))       # sdd = 8.0

        # create reference data for Y axis and compare to the output
        tof = dataX[:-1] + 5.25
        newY = self._input_ws.readY(0)*tof**3/(factor*10.5*12.0*12.0)
        self.assertTrue(np.allclose(newY, wsoutput.readY(0)))           # sdd = 4.0
        self.assertTrue(np.allclose(9.0*newY/16.0, wsoutput.readY(1)))       # sdd = 8.0

        DeleteWorkspace(wsoutput)

    def test_calculation_fitvanadium(self):
        OutputWorkspaceName = "outputws"
        alg_test = run_algorithm("TOFTOFConvertTofToDeltaE", InputWorkspace=self._input_ws,
                                 VanadiumWorkspace=self._input_ws, OutputWorkspace=OutputWorkspaceName,
                                 ChoiceElasticTof='FitVanadium')
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)

        # create reference data for X axis
        dataX = np.linspace(4005.75, 7995.75, 381)
        tel = 6005.25
        factor = m_n*1e+15/eV
        newX = 0.5*factor*12.0*12.0*(1/tel**2 - 1/dataX**2)
        # compare
        self.assertTrue(np.allclose(newX, wsoutput.readX(0)))           # sdd = 4.0
        self.assertTrue(np.allclose(16.0*newX/9.0, wsoutput.readX(1)))       # sdd = 8.0

        # create reference data for Y axis and compare to the output
        tof = dataX[:-1] + 5.25
        newY = self._input_ws.readY(0)*tof**3/(factor*10.5*12.0*12.0)
        self.assertTrue(np.allclose(newY, wsoutput.readY(0)))           # sdd = 4.0
        self.assertTrue(np.allclose(9.0*newY/16.0, wsoutput.readY(1)))       # sdd = 8.0

        DeleteWorkspace(wsoutput)

    def test_invalid_epp(self):
        AddSampleLog(self._input_ws, LogName='EPP', LogText=str(0), LogType='Number')
        OutputWorkspaceName = "outputws"
        self.assertRaises(RuntimeError, TOFTOFConvertTofToDeltaE, InputWorkspace=self._input_ws,
                          OutputWorkspace=OutputWorkspaceName)

    def test_no_channel_width(self):
        DeleteLog(self._input_ws, 'channel_width')
        OutputWorkspaceName = "outputws"
        self.assertRaises(RuntimeError, TOFTOFConvertTofToDeltaE, InputWorkspace=self._input_ws,
                          OutputWorkspace=OutputWorkspaceName)

    def cleanUp(self):
        if self._input_ws is not None:
            DeleteWorkspace(self._input_ws)

if __name__ == "__main__":
    unittest.main()
