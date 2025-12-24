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
        reduction_script = self.generate_reduction_script()
        execute_script(reduction_script)

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.nanEqual = True
        self.disableChecking = ["Axes"]
        return [
            "ws_Van_res_sqw",
            "TOFTOF_Van_1214_processed.nxs",
            "ws_EC_sqw",
            "TOFTOF_EC_1517_processed.nxs",
            "ws_H2O_21C_sqw",
            "TOFTOF_Sample_2729_processed.nxs",
            "ws_H2O_34C_sqw",
            "TOFTOF_Sample_3031_processed.nxs",
        ]

    def generate_reduction_script(self):
        scripter = TOFTOFScriptElement()

        # set general parameters
        scripter.facility_name = config["default.facility"]
        scripter.instrument_name = config["default.instrument"]
        scripter.dataDir = ""
        scripter.prefix = "ws"

        # vanadium inputs
        scripter.vanRuns = "TOFTOF12:14"
        scripter.vanCmnt = "Van_res"
        scripter.vanTemp = 21
        scripter.vanEcFactor = 1.0

        # empty container inputs
        scripter.ecRuns = "TOFTOF15:17"
        scripter.ecComment = "EC"
        scripter.ecTemp = 21
        scripter.ecFactor = 0.9

        # data inputs
        scripter.dataRuns = [["TOFTOF27:29", "H2O_21C", 21], ["TOFTOF30:31", "H2O_34C", 34]]

        # masking
        scripter.maskDetectors = "190, 294, 419, 485, 494, 502, 527, 614, 646, 665, 952, 965"

        # binning
        scripter.binEon = True
        scripter.binEstart = -6.0
        scripter.binEstep = 0.01
        scripter.binEend = 1.8

        scripter.binQon = True
        scripter.binQstart = 0.4
        scripter.binQstep = 0.1
        scripter.binQend = 2.0

        # options
        scripter.subtractECVan = True
        scripter.normalise = 1  # 0: none, 1: to monitor, 2: to time
        scripter.correctTof = 1  # 0: none, 1: vana, 2: sample

        scripter.replaceNaNs = False
        scripter.createDiff = False
        scripter.keepSteps = False

        # save reduced data options
        scripter.saveSofTWNxspe = False
        scripter.saveSofTWNexus = False
        scripter.saveSofTWAscii = False
        scripter.saveSofQWNexus = False
        scripter.saveSofQWAscii = False

        # generate reduction script
        reduction_script = scripter.to_script()
        return reduction_script
