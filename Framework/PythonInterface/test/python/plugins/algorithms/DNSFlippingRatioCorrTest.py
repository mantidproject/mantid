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
from mantid.simpleapi import DNSFlippingRatioCorr


class DNSFlippingRatioCorrTest(unittest.TestCase):
    workspaces = []
    __sf_bkgrws = None
    __nsf_bkgrws = None
    __sf_nicrws = None
    __nsf_nicrws = None

    def setUp(self):
        dataY = np.array(
            [
                2997.0,
                2470.0,
                2110.0,
                1818.0,
                840.0,
                1095.0,
                944.0,
                720.0,
                698.0,
                699.0,
                745.0,
                690.0,
                977.0,
                913.0,
                906.0,
                1007.0,
                1067.0,
                1119.0,
                1467.0,
                1542.0,
                2316.0,
                1536.0,
                1593.0,
                1646.0,
            ]
        )
        self.__sf_bkgrws = create_fake_dns_workspace("__sf_bkgrws", dataY=dataY / 620.0, flipper="ON")
        self.workspaces.append("__sf_bkgrws")
        dataY = np.array(
            [
                14198.0,
                7839.0,
                4386.0,
                3290.0,
                1334.0,
                1708.0,
                1354.0,
                1026.0,
                958.0,
                953.0,
                888.0,
                847.0,
                1042.0,
                1049.0,
                1012.0,
                1116.0,
                1294.0,
                1290.0,
                1834.0,
                1841.0,
                2740.0,
                1750.0,
                1965.0,
                1860.0,
            ]
        )
        self.__nsf_bkgrws = create_fake_dns_workspace("__nsf_bkgrws", dataY=dataY / 619.0, flipper="OFF")
        self.workspaces.append("__nsf_bkgrws")
        dataY = np.array(
            [
                5737.0,
                5761.0,
                5857.0,
                5571.0,
                4722.0,
                5102.0,
                4841.0,
                4768.0,
                5309.0,
                5883.0,
                5181.0,
                4455.0,
                4341.0,
                4984.0,
                3365.0,
                4885.0,
                4439.0,
                4103.0,
                4794.0,
                14760.0,
                10516.0,
                4445.0,
                5460.0,
                3942.0,
            ]
        )
        self.__nsf_nicrws = create_fake_dns_workspace("__nsf_nicrws", dataY=dataY / 58.0, flipper="OFF")
        self.workspaces.append("__nsf_nicrws")
        dataY = np.array(
            [
                2343.0,
                2270.0,
                2125.0,
                2254.0,
                1534.0,
                1863.0,
                1844.0,
                1759.0,
                1836.0,
                2030.0,
                1848.0,
                1650.0,
                1555.0,
                1677.0,
                1302.0,
                1750.0,
                1822.0,
                1663.0,
                2005.0,
                4025.0,
                3187.0,
                1935.0,
                2331.0,
                2125.0,
            ]
        )
        self.__sf_nicrws = create_fake_dns_workspace("__sf_nicrws", dataY=dataY / 295.0, flipper="ON")
        self.workspaces.append("__sf_nicrws")

    def tearDown(self):
        for wsname in self.workspaces:
            if api.AnalysisDataService.doesExist(wsname):
                api.DeleteWorkspace(wsname)
        self.workspaces = []

    def test_DNSFlipperValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test2"
        self.assertRaises(
            RuntimeError,
            DNSFlippingRatioCorr,
            SFDataWorkspace=self.__nsf_nicrws.name(),
            NSFDataWorkspace=self.__nsf_nicrws.name(),
            SFNiCrWorkspace=self.__sf_nicrws.name(),
            NSFNiCrWorkspace=self.__nsf_nicrws.name(),
            SFBkgrWorkspace=self.__sf_bkgrws.name(),
            NSFBkgrWorkspace=self.__nsf_bkgrws.name(),
            SFOutputWorkspace=outputWorkspaceName + "SF",
            NSFOutputWorkspace=outputWorkspaceName + "NSF",
        )
        return

    def test_DNSPolarisationValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test3"
        api.AddSampleLog(Workspace=self.__nsf_nicrws, LogName="polarisation", LogText="y", LogType="String")
        self.assertRaises(
            RuntimeError,
            DNSFlippingRatioCorr,
            SFDataWorkspace=self.__sf_nicrws.name(),
            NSFDataWorkspace=self.__nsf_nicrws.name(),
            SFNiCrWorkspace=self.__sf_nicrws.name(),
            NSFNiCrWorkspace=self.__nsf_nicrws.name(),
            SFBkgrWorkspace=self.__sf_bkgrws.name(),
            NSFBkgrWorkspace=self.__nsf_bkgrws.name(),
            SFOutputWorkspace=outputWorkspaceName + "SF",
            NSFOutputWorkspace=outputWorkspaceName + "NSF",
        )
        return

    def test_DNSFRSelfCorrection(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test4"
        # consider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = self.__sf_nicrws - self.__sf_bkgrws
        dataws_nsf = self.__nsf_nicrws - self.__nsf_bkgrws
        alg_test = run_algorithm(
            "DNSFlippingRatioCorr",
            SFDataWorkspace=dataws_sf,
            NSFDataWorkspace=dataws_nsf,
            SFNiCrWorkspace=self.__sf_nicrws.name(),
            NSFNiCrWorkspace=self.__nsf_nicrws.name(),
            SFBkgrWorkspace=self.__sf_bkgrws.name(),
            NSFBkgrWorkspace=self.__nsf_bkgrws.name(),
            SFOutputWorkspace=outputWorkspaceName + "SF",
            NSFOutputWorkspace=outputWorkspaceName + "NSF",
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + "SF")
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + "NSF")
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2, ws_sf.getNumDims())
        self.assertEqual(2, ws_nsf.getNumDims())
        # data array: spin-flip must be zero
        for i in range(24):
            self.assertAlmostEqual(0.0, ws_sf.readY(i)[0])
        # data array: non spin-flip must be nsf - sf^2/nsf
        nsf = np.array(dataws_nsf.extractY())
        sf = np.array(dataws_sf.extractY())
        refdata = nsf + sf
        for i in range(24):
            self.assertAlmostEqual(refdata[i][0], ws_nsf.readY(i)[0])

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "SF")
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "NSF")
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        return

    def test_DNSTwoTheta(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test5"

        # rotate detector bank to different angles
        dataws_sf = self.__sf_nicrws - self.__sf_bkgrws
        dataws_nsf = self.__nsf_nicrws - self.__nsf_bkgrws
        wslist = [dataws_sf, dataws_nsf, self.__sf_nicrws, self.__nsf_nicrws, self.__sf_bkgrws, self.__nsf_bkgrws]
        for wks in wslist:
            api.LoadInstrument(wks, InstrumentName="DNS", RewriteSpectraMap=True)
        api.RotateInstrumentComponent(dataws_sf, "bank0", X=0, Y=1, Z=0, Angle=-7.53)
        api.RotateInstrumentComponent(dataws_nsf, "bank0", X=0, Y=1, Z=0, Angle=-7.53)
        api.RotateInstrumentComponent(self.__sf_nicrws, "bank0", X=0, Y=1, Z=0, Angle=-8.02)
        api.RotateInstrumentComponent(self.__nsf_nicrws, "bank0", X=0, Y=1, Z=0, Angle=-8.02)
        api.RotateInstrumentComponent(self.__sf_bkgrws, "bank0", X=0, Y=1, Z=0, Angle=-8.54)
        api.RotateInstrumentComponent(self.__nsf_bkgrws, "bank0", X=0, Y=1, Z=0, Angle=-8.54)
        # apply correction
        alg_test = run_algorithm(
            "DNSFlippingRatioCorr",
            SFDataWorkspace=dataws_sf,
            NSFDataWorkspace=dataws_nsf,
            SFNiCrWorkspace=self.__sf_nicrws.name(),
            NSFNiCrWorkspace=self.__nsf_nicrws.name(),
            SFBkgrWorkspace=self.__sf_bkgrws.name(),
            NSFBkgrWorkspace=self.__nsf_bkgrws.name(),
            SFOutputWorkspace=outputWorkspaceName + "SF",
            NSFOutputWorkspace=outputWorkspaceName + "NSF",
        )

        self.assertTrue(alg_test.isExecuted())
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + "SF")
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + "NSF")
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2, ws_sf.getNumDims())
        self.assertEqual(2, ws_nsf.getNumDims())
        # 2theta angles must not change after correction has been applied
        tthetas = np.array([7.53 + i * 5 for i in range(24)])
        for i in range(24):
            det = ws_sf.getDetector(i)
            self.assertAlmostEqual(tthetas[i], np.degrees(ws_sf.detectorSignedTwoTheta(det)))
            det = ws_nsf.getDetector(i)
            self.assertAlmostEqual(tthetas[i], np.degrees(ws_nsf.detectorSignedTwoTheta(det)))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "SF")
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "NSF")
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)

        return

    def test_DNSFRVanaCorrection(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test6"
        # create fake vanadium data workspaces
        dataY = np.array(
            [
                1811.0,
                2407.0,
                3558.0,
                3658.0,
                3352.0,
                2321.0,
                2240.0,
                2617.0,
                3245.0,
                3340.0,
                3338.0,
                3310.0,
                2744.0,
                3212.0,
                1998.0,
                2754.0,
                2791.0,
                2509.0,
                3045.0,
                3429.0,
                3231.0,
                2668.0,
                3373.0,
                2227.0,
            ]
        )
        __sf_vanaws = create_fake_dns_workspace("__sf_vanaws", dataY=dataY / 58.0, flipper="ON")
        self.workspaces.append("__sf_vanaws")
        dataY = np.array(
            [
                2050.0,
                1910.0,
                2295.0,
                2236.0,
                1965.0,
                1393.0,
                1402.0,
                1589.0,
                1902.0,
                1972.0,
                2091.0,
                1957.0,
                1593.0,
                1952.0,
                1232.0,
                1720.0,
                1689.0,
                1568.0,
                1906.0,
                2001.0,
                2051.0,
                1687.0,
                1975.0,
                1456.0,
            ]
        )
        __nsf_vanaws = create_fake_dns_workspace("__nsf_vanaws", dataY=dataY / 58.0, flipper="OFF")
        self.workspaces.append("__nsf_vanaws")
        # consider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = __sf_vanaws - self.__sf_bkgrws
        dataws_nsf = __nsf_vanaws - self.__nsf_bkgrws
        alg_test = run_algorithm(
            "DNSFlippingRatioCorr",
            SFDataWorkspace=dataws_sf,
            NSFDataWorkspace=dataws_nsf,
            SFNiCrWorkspace=self.__sf_nicrws.name(),
            NSFNiCrWorkspace=self.__nsf_nicrws.name(),
            SFBkgrWorkspace=self.__sf_bkgrws.name(),
            NSFBkgrWorkspace=self.__nsf_bkgrws.name(),
            SFOutputWorkspace=outputWorkspaceName + "SF",
            NSFOutputWorkspace=outputWorkspaceName + "NSF",
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + "SF")
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + "NSF")
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2, ws_sf.getNumDims())
        self.assertEqual(2, ws_nsf.getNumDims())
        # data array: for vanadium ratio sf/nsf must be around 2
        ws = ws_sf / ws_nsf
        for i in range(24):
            self.assertAlmostEqual(2.0, np.around(ws.readY(i)))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "SF")
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + "NSF")
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        run_algorithm("DeleteWorkspace", Workspace=ws)
        return


if __name__ == "__main__":
    unittest.main()
