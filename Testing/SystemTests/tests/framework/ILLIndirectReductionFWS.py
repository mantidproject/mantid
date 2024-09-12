# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import CompareWorkspaces, LoadNexusProcessed, IndirectILLReductionFWS
from mantid import config, mtd
import numpy


class ILLIndirectReductionFWSTest(systemtesting.MantidSystemTest):
    # cache default instrument and datadirs
    facility = config["default.facility"]
    instrument = config["default.instrument"]
    datadirs = config["datasearch.directories"]

    def __init__(self):
        super(ILLIndirectReductionFWSTest, self).__init__()
        self.setUp()

    def setUp(self):
        # these must be set, so the required files
        # without instrument name can be retrieved
        config["default.facility"] = "ILL"
        config["default.instrument"] = "IN16B"
        config.appendDataSearchSubDir("ILL/IN16B/")

        self.params = {"Tolerance": 1e-3, "CheckInstrument": False}

    def tearDown(self):
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.datadirs

    def requiredFiles(self):
        return [
            "165944.nxs",
            "165945.nxs",
            "165946.nxs",
            "165947.nxs",
            "165948.nxs",
            "165949.nxs",
            "165950.nxs",
            "165951.nxs",
            "165952.nxs",
            "165953.nxs",
            "143720.nxs",
            "143721.nxs",
            "143722.nxs",
            "143723.nxs",
            "143724.nxs",
            "143725.nxs",
            "143726.nxs",
            "143727.nxs",
            "143728.nxs",
            "143729.nxs",
            "140678.nxs",
            "140679.nxs",
            "140680.nxs",
            "140681.nxs",
            "140682.nxs",
        ]

    def runTest(self):
        self._run_ifws()

        self._run_efws()

        self._run_sum_interpolate()

        self._run_efws_mirror_sense()

        self._run_calib_bg()

        self.tearDown()

    def _run_ifws(self):
        # test EFWS+IFWS mixed
        IndirectILLReductionFWS(Run="165944:165953", SortXAxis=True, OutputWorkspace="ifws")

        LoadNexusProcessed(Filename="ILLIN16B_FWS.nxs", OutputWorkspace="ref")

        result = CompareWorkspaces(Workspace1="ifws_red", Workspace2="ref", **self.params)

        if result[0]:
            self.assertTrue(result[0])
        else:
            self.assertTrue(result[0], "Mismatch in IFWS: " + result[1].row(0)["Message"])

    def _run_efws(self):
        # test EFWS with sum/interpolate options with background and calibration
        IndirectILLReductionFWS(
            Run="143720:143728:2",
            BackgroundRun="143721,143723,143725",
            CalibrationRun="143727,143729",
            BackgroundOption="Interpolate",
            CalibrationOption="Sum",
            SortXAxis=True,
            OutputWorkspace="efws",
        )

        LoadNexusProcessed(Filename="ILLIN16B_EFWS.nxs", OutputWorkspace="ref")

        result = CompareWorkspaces(Workspace1="efws_0.0_red", Workspace2="ref", **self.params)

        if result[0]:
            self.assertTrue(result[0])
        else:
            self.assertTrue(result[0], "Mismatch in EFWS: " + result[1].row(0)["Message"])

    def _run_sum_interpolate(self):
        # this cross-tests if only one background point is given,
        # sum and interpolate options should give identical output
        IndirectILLReductionFWS(
            Run="143720:143728:2", BackgroundRun="143721", BackgroundOption="Interpolate", SortXAxis=True, OutputWorkspace="efws_int"
        )

        IndirectILLReductionFWS(
            Run="143720:143728:2", BackgroundRun="143721", BackgroundOption="Sum", SortXAxis=True, OutputWorkspace="efws_sum"
        )

        result = CompareWorkspaces(Workspace1="efws_int_red", Workspace2="efws_sum_red", Tolerance=1e-9)

        if result[0]:
            self.assertTrue(result[0])
        else:
            self.assertTrue(result[0], "Sum/interpolate should be the same for one point: " + result[1].row(0)["Message"])

    def _run_efws_mirror_sense(self):
        # this tests the EFWS in mirror mode: data in 140680 is indeed split to two wings,
        # while the others have right wing empty (though mirror sense is ON!)
        IndirectILLReductionFWS(Run="140678:140682", OutputWorkspace="efws_mirror")
        yData = mtd["efws_mirror_red"].getItem(0).readY(17)
        avg = numpy.average(yData)
        for y in numpy.nditer(yData):
            self.assertDelta(y, avg, 0.001)

    def _run_calib_bg(self):
        # this tests with the background file for calibration
        IndirectILLReductionFWS(
            Run="143720:143728:2",
            CalibrationRun="143721",
            CalibrationBackgroundRun="143723",
            CalibrationBackgroundScalingFactor=0.1,
            OutputWorkspace="efws_calib_bg",
        )

        self.assertEqual(mtd["efws_calib_bg_red"].getItem(0).getNumberHistograms(), 18)

        self.assertDelta(mtd["efws_calib_bg_red"].getItem(0).readY(0)[0], 0.218, 0.001)
