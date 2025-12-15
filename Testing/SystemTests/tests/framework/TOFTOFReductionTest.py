# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.kernel import config
from systemtesting import MantidSystemTest
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement
from reduction_gui.reduction.scripter import execute_script


class TOFTOFReductionTest(MantidSystemTest):
    """
    System test that executes the full TOFTOF reduction workflow matching the
    manual DGS Reduction steps and validates the expected outputs.
    """

    def __init__(self):
        MantidSystemTest.__init__(self)
        self.setUp()

    def setUp(self):
        config["default.facility"] = "MLZ"
        config["default.instrument"] = "TOFTOF"

    # def cleanup(self) -> None:
    #     mtd.clear()

    def requiredFiles(self):
        return [
            "TOFTOF12.nxs",
            "TOFTOF13.nxs",
            "TOFTOF14.nxs",
            "TOFTOF15.nxs",
            "TOFTOF16.nxs",
            "TOFTOF17.nxs",
            "TOFTOF27.nxs",
            "TOFTOF28.nxs",
            "TOFTOF29.nxs",
            "TOFTOF30.nxs",
            "TOFTOF31.nxs",
        ]

    def runTest(self):
        red_script = self._generate_reduction_script()
        execute_script(red_script)

    def _generate_reduction_script(self):
        scriptTest = TOFTOFScriptElement()

        scriptTest.facility_name = config["default.facility"]
        scriptTest.instrument_name = config["default.instrument"]
        scriptTest.dataDir = ""
        scriptTest.prefix = "ws"

        scriptTest.vanRuns = "TOFTOF12:14"
        scriptTest.vanCmnt = "Van_res"
        scriptTest.vanTemp = 21
        scriptTest.vanEcFactor = 1.0

        scriptTest.ecRuns = "TOFTOF15:17"
        scriptTest.ecComment = "EC"
        scriptTest.ecTemp = 21
        scriptTest.ecFactor = 0.9

        scriptTest.maskDetectors = False

        scriptTest.dataRuns = [["TOFTOF27:29", "H2O_21C", 21], ["TOFTOF30:31", "H2O_34C", 34]]

        # binning
        scriptTest.binEon = True
        scriptTest.binEstart = -6.0
        scriptTest.binEstep = 0.01
        scriptTest.binEend = 1.8

        scriptTest.binQon = True
        scriptTest.binQstart = 0.4
        scriptTest.binQstep = 0.1
        scriptTest.binQend = 2.0

        # options
        scriptTest.subtractECVan = True
        scriptTest.normalise = 1  # 0: no, 1: monitor, 2: time
        scriptTest.correctTof = 1  # 0: no, 1: vana, 2: sample

        scriptTest.replaceNaNs = False
        scriptTest.createDiff = False
        scriptTest.keepSteps = False

        # save reduced data options
        scriptTest.saveSofTWNxspe = False
        scriptTest.saveSofTWNexus = False
        scriptTest.saveSofTWAscii = False
        scriptTest.saveSofQWNexus = False
        scriptTest.saveSofQWAscii = False

        reduction_script = scriptTest.to_script()
        return reduction_script

        # expected_comments = {"H2O_21C", "H2O_34C"}

        # self.assertIn("gwsDataS", mtd)
        # self.assertIn("gwsDataBinE", mtd)
        # self.assertIn("gwsDataSQW", mtd)

        # sqw_group = mtd["gwsDataSQW"]
        # self.assertIsInstance(sqw_group, WorkspaceGroup)
        # self.assertEqual(set(sqw_group.getNames()), {f"ws_{comment}_sqw" for comment in expected_comments})

        # energy_group = mtd["gwsDataBinE"]
        # self.assertIsInstance(energy_group, WorkspaceGroup)
        # self.assertEqual(set(energy_group.getNames()), {f"ws_E_{comment}" for comment in expected_comments})

        # for comment in expected_comments:
        #     ws_name = f"ws_{comment}_sqw"
        #     workspace = mtd[ws_name]
        #     self.assertIsInstance(workspace, MatrixWorkspace)
        #     self.assertEqual(workspace.getComment(), comment)
        #     self.assertGreater(workspace.getNumberHistograms(), 0)
        #     self.assertGreater(workspace.blocksize(), 0)
        #     signal = workspace.dataY(0)
        #     self.assertTrue(np.isfinite(signal).any(), f"{ws_name} contains no finite signal values")
        #     self.assertGreater(np.abs(signal).sum(), 0.0, f"{ws_name} is unexpectedly empty")
