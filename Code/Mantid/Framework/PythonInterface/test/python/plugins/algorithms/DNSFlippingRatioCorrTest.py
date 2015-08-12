import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSFlippingRatioCorr


class DNSFlippingRatioCorrTest(unittest.TestCase):
    workspaces = []
    __sf_bkgrws = None
    __nsf_bkgrws = None
    __sf_nicrws = None
    __nsf_nicrws = None

    def _create_fake_workspace(self, wsname, dataY, flipper):
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
        p_values = '-7.53,4.2,10,10,5,20,x,' + flipper
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
        dataY = np.array([3054., 5774., 4350., 2782., 1861., 1727., 1501., 1292., 1440., 1481.,
                          1476., 1538., 1579., 1705., 1609., 1759., 1872., 1733., 2369., 2512.,
                          4000., 2767., 3223., 3645.])
        self.__sf_bkgrws = self._create_fake_workspace('__sf_bkgrws', dataY/1500.0, flipper='ON')
        self.workspaces.append('__sf_bkgrws')
        dataY = np.array([8206., 10521., 6835., 5555., 4203., 4547., 4049., 3348., 3407., 3565.,
                          3001., 2903., 2711., 2891., 2346., 2727., 2736., 2593., 3374., 4128.,
                          4900., 3218., 3901., 7121.])
        self.__nsf_bkgrws = self._create_fake_workspace('__nsf_bkgrws', dataY/1500.0, flipper='OFF')
        self.workspaces.append('__nsf_bkgrws')
        dataY = np.array([16741., 159041., 165912., 167196., 157053., 160667., 158403., 163190.,
                          169999., 187151., 171538., 172532., 146847., 178688., 124909., 161835.,
                          158691., 148143., 159613., 174784., 167548., 147775., 172739., 119592.])
        self.__nsf_nicrws = self._create_fake_workspace('__nsf_nicrws', dataY/600.0, flipper='OFF')
        self.workspaces.append('__nsf_nicrws')
        dataY = np.array([11069., 47736., 42658., 42974., 33120., 35430., 33967., 34085., 36585.,
                         41440., 41903., 38295., 27700., 38924., 24908., 31017., 32980., 29603.,
                         32653., 35552., 34977., 44234., 39729., 45156.])
        self.__sf_nicrws = self._create_fake_workspace('__sf_nicrws', dataY/3000.0, flipper='ON')
        self.workspaces.append('__sf_nicrws')

    def tearDown(self):
        for wsname in self.workspaces:
            if api.AnalysisDataService.doesExist(wsname + '_NORM'):
                api.DeleteWorkspace(wsname + '_NORM')
            if api.AnalysisDataService.doesExist(wsname):
                api.DeleteWorkspace(wsname)
        self.workspaces = []

    def test_DNSNormWorkspaceExists(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test1"
        api.DeleteWorkspace(self.__sf_bkgrws.getName() + '_NORM')
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__sf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSFlipperValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test2"
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__nsf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSPolarisationValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test3"
        api.AddSampleLog(Workspace=self.__nsf_nicrws, LogName='polarisation', LogText='y', LogType='String')
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__sf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSFRSelfCorrection(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test4"
        # cosider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = self.__sf_nicrws - self.__sf_bkgrws
        dataws_nsf = self.__nsf_nicrws - self.__nsf_bkgrws
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF')

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + 'SF')
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF')
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2,  ws_sf.getNumDims())
        self.assertEqual(2,  ws_nsf.getNumDims())
        # data array spin flip must be zero
        for i in range(24):
            self.assertAlmostEqual(0.0, ws_sf.readY(i))
        # data array non spin-flip must be nsf - sf^2/nsf
        nsf = np.array(dataws_nsf.extractY()[0])
        sf = np.array(dataws_sf.extractY()[0])
        refdata = nsf - sf*sf/nsf
        for i in range(24):
            self.assertAlmostEqual(refdata, ws_nsf.readY(i))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF')
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        return


if __name__ == '__main__':
    unittest.main()
