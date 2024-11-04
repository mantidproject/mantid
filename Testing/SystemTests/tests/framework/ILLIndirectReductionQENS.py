# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import IndirectILLReductionQENS, Plus, CompareWorkspaces, GroupWorkspaces, Scale
from mantid import config, mtd


class ILLIndirectReductionQENSTest(systemtesting.MantidSystemTest):
    # cache default instrument and datadirs
    facility = config["default.facility"]
    instrument = config["default.instrument"]
    datadirs = config["datasearch.directories"]

    def __init__(self):
        super(ILLIndirectReductionQENSTest, self).__init__()
        self.setUp()

    def setUp(self):
        # these must be set, so the required files
        # without instrument name can be retrieved
        config["default.facility"] = "ILL"
        config["default.instrument"] = "IN16B"
        config.appendDataSearchSubDir("ILL/IN16B/")

    def tearDown(self):
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.datadirs

    def test_unmirror_0_1_2_3(self):
        args = {"Run": "136553.nxs", "UnmirrorOption": 0, "OutputWorkspace": "zero"}

        IndirectILLReductionQENS(**args)

        args["UnmirrorOption"] = 1

        args["OutputWorkspace"] = "both"

        IndirectILLReductionQENS(**args)

        args["UnmirrorOption"] = 2

        args["OutputWorkspace"] = "left"

        IndirectILLReductionQENS(**args)

        args["UnmirrorOption"] = 3

        args["OutputWorkspace"] = "right"

        IndirectILLReductionQENS(**args)

        summed = Plus(mtd["left_red"].getItem(0), mtd["right_red"].getItem(0))

        Scale(InputWorkspace=summed, Factor=0.5, OutputWorkspace=summed)

        result = CompareWorkspaces(summed, mtd["both_red"].getItem(0))

        self.assertTrue(result[0], "Unmirror 1 should be the sum of 2 and 3")

        left_right = GroupWorkspaces([mtd["left_red"].getItem(0).name(), mtd["right_red"].getItem(0).name()])

        result = CompareWorkspaces(left_right, "zero_red")

        self.assertTrue(result[0], "Unmirror 0 should be the group of 2 and 3")

    def test_unmirror_4_5(self):
        args = {"Run": "136553.nxs", "UnmirrorOption": 4, "OutputWorkspace": "vana4"}

        IndirectILLReductionQENS(**args)

        args["AlignmentRun"] = "136553.nxs"

        args["UnmirrorOption"] = 5

        args["OutputWorkspace"] = "vana5"

        IndirectILLReductionQENS(**args)

        result = CompareWorkspaces("vana4_red", "vana5_red")

        self.assertTrue(result[0], "Unmirror 4 should be the same as 5 if " "the same run is also defined as alignment run")

    def test_unmirror_6_7(self):
        args = {"Run": "136553.nxs", "UnmirrorOption": 6, "OutputWorkspace": "vana6"}

        IndirectILLReductionQENS(**args)

        args["AlignmentRun"] = "136553.nxs"

        args["UnmirrorOption"] = 7

        args["OutputWorkspace"] = "vana7"

        IndirectILLReductionQENS(**args)

        result = CompareWorkspaces("vana6_red", "vana7_red")

        self.assertTrue(result[0], "Unmirror 6 should be the same as 7 if " "the same run is also defined as alignment run")

    def test_unmirror_7_same_reference(self):
        # there was a bug in MatchPeaks with InputWorkspace2 and center peaks ON
        # in the case where the fitting was failing because of a too narrow peak
        # this tests the fix with the data where there are too narrow peaks to fit
        args = {"AlignmentRun": "251554", "UnmirrorOption": 7, "ManualPSDIntegrationRange": "44,90"}

        args["Run"] = "251530"
        args["OutputWorkspace"] = "50K"
        IndirectILLReductionQENS(**args)
        args["Run"] = "251548"
        args["OutputWorkspace"] = "200K"
        IndirectILLReductionQENS(**args)

        # 2 workspaces are aligned with the same vanadium, so they must have the same
        # amount of masked bins in the beginning and in the end of the spectra
        m1 = mtd["50K_red"].getItem(0).maskedBinIndices(0)
        m2 = mtd["200K_red"].getItem(0).maskedBinIndices(0)
        self.assertEqual(len(m1), len(m2))
        for i in range(len(m1)):
            self.assertEqual(m1[i], m2[i])

    def runTest(self):
        self.test_unmirror_0_1_2_3()

        self.test_unmirror_4_5()

        self.test_unmirror_6_7()

        self.runTestBackgroundCalibration()

        self.runTestDifferentZeroMonitorChannels()

        self.tolerance = 1e-3

        self.disableChecking = ["Instrument"]

        IndirectILLReductionQENS(
            Run="136558-136559",
            CalibrationRun="136553-136554",
            BackgroundRun="136599-136600",
            AlignmentRun="136555-136556",
            BackgroundScalingFactor=0.1,
            UnmirrorOption=7,
            OutputWorkspace="out",
        )

        self.tearDown()

    def runTestBackgroundCalibration(self):
        IndirectILLReductionQENS(
            Run="136558",
            CalibrationRun="136553",
            CalibrationBackgroundRun="136599",
            CalibrationBackgroundScalingFactor=0.1,
            OutputWorkspace="out_calib_bg",
        )

        self.assertEqual(mtd["out_calib_bg_red"].getItem(0).getNumberHistograms(), 18)

        self.assertDelta(mtd["out_calib_bg_red"].getItem(0).readY(0)[1024 - 580], 0.0035, 0.0001)

    def runTestDifferentZeroMonitorChannels(self):
        out_croped_mon = IndirectILLReductionQENS(Run="140721-140722", CropDeadMonitorChannels=True)

        self.assertEqual(out_croped_mon.getItem(0).getNumberHistograms(), 18)

        self.assertEqual(out_croped_mon.getItem(0).blocksize(), 1018)

    def validate(self):
        return ["136558_multiple_out_red", "ILLIN16B_QENS.nxs"]
