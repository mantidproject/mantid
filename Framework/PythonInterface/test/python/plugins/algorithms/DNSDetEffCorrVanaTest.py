import unittest
from testhelpers import run_algorithm
from testhelpers.mlzhelpers import create_fake_dns_workspace
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSDetEffCorrVana


class DNSDetEffCorrVanaTest(unittest.TestCase):
    __dataws = None
    __vanaws = None
    __bkgrws = None

    def setUp(self):
        dataY = np.zeros(24)
        dataY.fill(1.5)
        self.__bkgrws = create_fake_dns_workspace('__bkgrws', dataY=dataY)
        dataY = np.linspace(2, 13.5, 24)
        self.__vanaws = create_fake_dns_workspace('__vanaws', dataY=dataY)
        dataY = np.arange(1, 25)
        self.__dataws = create_fake_dns_workspace('__dataws', dataY=dataY)

    def tearDown(self):
        api.DeleteWorkspace(self.__vanaws.getName() + '_NORM')
        api.DeleteWorkspace(self.__dataws.getName() + '_NORM')
        if api.mtd.doesExist(self.__bkgrws.getName() + '_NORM'):
            api.DeleteWorkspace(self.__bkgrws.getName() + '_NORM')
        api.DeleteWorkspace(self.__bkgrws)
        api.DeleteWorkspace(self.__vanaws)
        api.DeleteWorkspace(self.__dataws)

    def test_DNSNormWorkspaceExists(self):
        outputWorkspaceName = "DNSDetCorrVanaTest_Test1"
        api.DeleteWorkspace(self.__bkgrws.getName() + '_NORM')
        self.assertRaises(RuntimeError, DNSDetEffCorrVana, InputWorkspace=self.__dataws.getName(),
                          OutputWorkspace=outputWorkspaceName, VanaWorkspace=self.__vanaws.getName(),
                          BkgWorkspace=self.__bkgrws.getName())
        return

    def test_VanaMeanDimensions(self):
        outputWorkspaceName = "DNSDetCorrVanaTest_Test2"
        _dataT_ = api.Transpose(self.__dataws)    # correct dimensions, but wrong number of bins
        self.assertRaises(RuntimeError, DNSDetEffCorrVana, InputWorkspace=self.__dataws.getName(),
                          OutputWorkspace=outputWorkspaceName, VanaWorkspace=self.__vanaws.getName(),
                          BkgWorkspace=self.__bkgrws.getName(), VanadiumMean=_dataT_)
        api.DeleteWorkspace(_dataT_)
        return

    def test_DNSVanadiumCorrection(self):
        outputWorkspaceName = "DNSDetCorrVanaTest_Test3"
        alg_test = run_algorithm("DNSDetEffCorrVana", InputWorkspace=self.__dataws.getName(),
                                 OutputWorkspace=outputWorkspaceName, VanaWorkspace=self.__vanaws.getName(),
                                 BkgWorkspace=self.__bkgrws.getName())

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        for i in range(24):
            self.assertAlmostEqual(12.5, ws.readY(i))
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_NegativeValues(self):
        outputWorkspaceName = "DNSDetCorrVanaTest_Test4"
        self.assertRaises(RuntimeError, DNSDetEffCorrVana, InputWorkspace=self.__dataws.getName(),
                          OutputWorkspace=outputWorkspaceName, VanaWorkspace=self.__bkgrws.getName(),
                          BkgWorkspace=self.__vanaws.getName())
        return

    def test_TwoTheta(self):
        # check whether the 2theta angles the same as in the data workspace
        outputWorkspaceName = "DNSDetCorrVanaTest_Test5"
        # rotate detector bank to different angles
        api.LoadInstrument(self.__dataws, InstrumentName='DNS')
        api.LoadInstrument(self.__vanaws, InstrumentName='DNS')
        api.LoadInstrument(self.__bkgrws, InstrumentName='DNS')

        api.RotateInstrumentComponent(self.__dataws, "bank0", X=0, Y=1, Z=0, Angle=-7.53)
        api.RotateInstrumentComponent(self.__vanaws, "bank0", X=0, Y=1, Z=0, Angle=-8.02)
        api.RotateInstrumentComponent(self.__bkgrws, "bank0", X=0, Y=1, Z=0, Angle=-8.54)
        # run correction
        alg_test = run_algorithm("DNSDetEffCorrVana", InputWorkspace=self.__dataws.getName(),
                                 OutputWorkspace=outputWorkspaceName, VanaWorkspace=self.__vanaws.getName(),
                                 BkgWorkspace=self.__bkgrws.getName())
        self.assertTrue(alg_test.isExecuted())
        # check dimensions and angles
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # angles
        tthetas = np.array([7.53 + i*5 for i in range(24)])
        for i in range(24):
            det = ws.getDetector(i)
            self.assertAlmostEqual(tthetas[i], np.degrees(ws.detectorSignedTwoTheta(det)))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

if __name__ == '__main__':
    unittest.main()
