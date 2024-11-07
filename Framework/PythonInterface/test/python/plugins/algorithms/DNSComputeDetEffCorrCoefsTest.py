# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import run_algorithm
from testhelpers.mlzhelpers import create_fake_dns_workspace
from mantid.api import AnalysisDataService, WorkspaceGroup
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSComputeDetEffCorrCoefs, MaskDetectors, GroupWorkspaces


class DNSComputeDetEffCorrCoefsTest(unittest.TestCase):
    sfvanaws = None
    nsfvanaws = None
    sfbkgrws = None
    nsfbkgrws = None

    def setUp(self):
        dataY = np.zeros(24)
        dataY.fill(1.5)
        self.sfbkgrws = create_fake_dns_workspace("sfbkgrws", dataY=dataY, flipper="ON", angle=-7.53, loadinstrument=True)
        self.nsfbkgrws = create_fake_dns_workspace("nsfbkgrws", dataY=dataY, flipper="OFF", angle=-7.53, loadinstrument=True)
        dataY = np.linspace(2, 13.5, 24)
        self.sfvanaws = create_fake_dns_workspace("sfvanaws", dataY=dataY, flipper="ON", angle=-7.53, loadinstrument=True)
        self.nsfvanaws = create_fake_dns_workspace("nsfvanaws", dataY=dataY, flipper="OFF", angle=-7.53, loadinstrument=True)

    def tearDown(self):
        for wks in [self.sfvanaws, self.nsfvanaws, self.sfbkgrws, self.nsfbkgrws]:
            api.DeleteWorkspace(wks)

    def test_DNSVanadiumCorrection(self):
        outputWorkspaceName = "DNSComputeDetCorrCoefsTest_Test1"
        vanalist = [self.sfvanaws.name(), self.nsfvanaws.name()]
        bglist = [self.sfbkgrws.name(), self.nsfbkgrws.name()]
        alg_test = run_algorithm(
            "DNSComputeDetEffCorrCoefs", VanadiumWorkspaces=vanalist, BackgroundWorkspaces=bglist, OutputWorkspace=outputWorkspaceName
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2, ws.getNumDims())
        # reference data
        refdata = np.linspace(0.08, 1.92, 24)
        # data array
        for i in range(24):
            self.assertAlmostEqual(refdata[i], ws.readY(i)[0])
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_NegativeValues(self):
        outputWorkspaceName = "DNSComputeDetCorrCoefsTest_Test2"
        vanalist = [self.sfvanaws.name(), self.nsfvanaws.name()]
        bglist = [self.sfbkgrws.name(), self.nsfbkgrws.name()]
        self.assertRaisesRegex(
            RuntimeError,
            "Background sfvanaws is higher than Vanadium sfbkgrws signal!",
            DNSComputeDetEffCorrCoefs,
            VanadiumWorkspaces=bglist,
            BackgroundWorkspaces=vanalist,
            OutputWorkspace=outputWorkspaceName,
        )
        return

    def test_DNSVanadiumCorrection_Masked(self):
        outputWorkspaceName = "DNSComputeDetCorrCoefsTest_Test3"
        vanalist = [self.sfvanaws.name(), self.nsfvanaws.name()]
        bglist = [self.sfbkgrws.name(), self.nsfbkgrws.name()]
        MaskDetectors(self.sfvanaws, DetectorList=[1])
        MaskDetectors(self.nsfvanaws, DetectorList=[1])

        alg_test = run_algorithm(
            "DNSComputeDetEffCorrCoefs", VanadiumWorkspaces=vanalist, BackgroundWorkspaces=bglist, OutputWorkspace=outputWorkspaceName
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2, ws.getNumDims())
        # reference data
        refdata = np.linspace(1.0, 24, 24) / 13.0
        refdata[0] = 0  # detector is masked
        # data array
        for i in range(24):
            self.assertAlmostEqual(refdata[i], ws.readY(i))
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_DNSVanadiumCorrection_Groups(self):
        outputWorkspaceName = "DNSComputeDetCorrCoefsTest_Test1"
        dataY = np.linspace(2, 13.5, 24)
        sfvana2 = create_fake_dns_workspace("sfvana2", dataY=dataY, flipper="ON", angle=-8.54, loadinstrument=True)
        nsfvana2 = create_fake_dns_workspace("nsfvana2", dataY=dataY, flipper="OFF", angle=-8.54, loadinstrument=True)
        vanagroupsf = GroupWorkspaces([self.sfvanaws, sfvana2])
        vanagroupnsf = GroupWorkspaces([self.nsfvanaws, nsfvana2])

        dataY.fill(1.5)
        sfbg2 = create_fake_dns_workspace("sfbg2", dataY=dataY, flipper="ON", angle=-8.54, loadinstrument=True)
        nsfbg2 = create_fake_dns_workspace("nsfbg2", dataY=dataY, flipper="OFF", angle=-8.54, loadinstrument=True)
        bggroupsf = GroupWorkspaces([self.sfbkgrws, sfbg2])
        bggroupnsf = GroupWorkspaces([self.nsfbkgrws, nsfbg2])
        alg_test = run_algorithm(
            "DNSComputeDetEffCorrCoefs",
            VanadiumWorkspaces=[vanagroupsf, vanagroupnsf],
            BackgroundWorkspaces=[bggroupsf, bggroupnsf],
            OutputWorkspace=outputWorkspaceName,
        )

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        group = AnalysisDataService.retrieve(outputWorkspaceName)
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertEqual(2, group.getNumberOfEntries())
        res1 = group.getItem(0)
        res2 = group.getItem(1)
        # dimensions
        self.assertEqual(24, res1.getNumberHistograms())
        self.assertEqual(24, res2.getNumberHistograms())
        self.assertEqual(2, res1.getNumDims())
        self.assertEqual(2, res2.getNumDims())
        # reference data
        refdata = np.linspace(0.08, 1.92, 24)
        # data array
        for i in range(24):
            self.assertAlmostEqual(refdata[i], res1.readY(i)[0])
            self.assertAlmostEqual(refdata[i], res2.readY(i)[0])
        wslist = [outputWorkspaceName, "sfvana2", "nsfvana2", "sfbg2", "nsfbg2"]
        for wsname in wslist:
            run_algorithm("DeleteWorkspace", Workspace=wsname)
        return


if __name__ == "__main__":
    unittest.main()
