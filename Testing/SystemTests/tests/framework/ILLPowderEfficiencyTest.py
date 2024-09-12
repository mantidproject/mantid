# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import PowderILLEfficiency, GroupWorkspaces
from mantid import config, mtd


class ILLPowderEfficiencyTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILLPowderEfficiencyTest, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D20"
        config.appendDataSearchSubDir("ILL/D20/")

    def requiredFiles(self):
        return ["967076.nxs"]

    def cleanup(self):
        mtd.clear()

    def runTest(self):
        PowderILLEfficiency(CalibrationRun="967076.nxs", OutputWorkspace="calib", OutputResponseWorkspace="response")
        GroupWorkspaces(InputWorkspaces=["calib", "response"], OutputWorkspace="group")

    def validate(self):
        self.tolerance = 0.0001
        return ["group", "ILL_D20_calib_def.nxs"]


class ILLPowderEfficiencyCycle203Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILLPowderEfficiencyCycle203Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D20"
        config.appendDataSearchSubDir("ILL/D20/")

    def cleanup(self):
        mtd.clear()

    def runTest(self):
        PowderILLEfficiency(
            CalibrationRun="167339", OutputWorkspace="calib", InterpolateOverlappingAngles=True, OutputResponseWorkspace="response"
        )
        GroupWorkspaces(InputWorkspaces=["calib", "response"], OutputWorkspace="group")

    def validate(self):
        self.tolerance = 0.0001
        return ["group", "ILL_D20_calib_203.nxs"]
