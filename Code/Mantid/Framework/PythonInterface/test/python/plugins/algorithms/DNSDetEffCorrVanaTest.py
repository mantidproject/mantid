import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSDetEffCorrVana


class DNSDetEffCorrVanaTest(unittest.TestCase):
    __dataws = None
    __vanaws = None
    __bkgrws = None

    def _create_fake_workspace(self, wsname, dataY):
        """
        creates DNS workspace with fake data
        """
        ndet = 24
        dataX = np.zeros(2*ndet)
        dataX.fill(4.2 + 0.00001)
        dataX[::2] -= 0.000002
        dataE = np.sqrt(dataY)
        # create workspace
        api.CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        outws = api.mtd[wsname]
        api.LoadInstrument(outws, InstrumentName='DNS')
        p_names = 'deterota,wavelength,slit_i_left_blade_position,slit_i_right_blade_position,\
            slit_i_lower_blade_position,slit_i_upper_blade_position,polarisation,flipper'
        p_values = '-7.53,4.2,10,10,5,20,x,ON'
        api.AddSampleLogMultiple(Workspace=outws, LogNames=p_names, LogValues=p_values, ParseType=True)
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
        dataY = np.zeros(24)
        dataY.fill(1.5)
        self.__bkgrws = self._create_fake_workspace('__bkgrws', dataY)
        dataY = np.linspace(2, 13.5, 24)
        self.__vanaws = self._create_fake_workspace('__vanaws', dataY)
        dataY = np.arange(1, 25)
        self.__dataws = self._create_fake_workspace('__dataws', dataY)

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


if __name__ == '__main__':
    unittest.main()
