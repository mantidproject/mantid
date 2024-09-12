# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from tempfile import gettempdir
from os import path, remove
import systemtesting
from mantid.simpleapi import PowderILLEfficiency, SaveNexusProcessed
from mantid import config, mtd


class ILLPowderEfficiencyClosureTest(systemtesting.MantidSystemTest):
    _m_tmp_file = None

    def __init__(self):
        super(ILLPowderEfficiencyClosureTest, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D20"
        config.appendDataSearchSubDir("ILL/D20/")
        self._m_tmp_file = path.join(gettempdir(), "D20Calib1stIteration.nxs")

    def requiredFiles(self):
        return ["967076.nxs"]

    def cleanup(self):
        mtd.clear()
        remove(self._m_tmp_file)

    def runTest(self):
        PowderILLEfficiency(CalibrationRun="967076.nxs", OutputWorkspace="calib")

        SaveNexusProcessed(InputWorkspace="calib", Filename=self._m_tmp_file)

        PowderILLEfficiency(CalibrationRun="967076.nxs", CalibrationFile=self._m_tmp_file, OutputWorkspace="calib-2nd")

        for i in range(mtd["calib-2nd"].getNumberHistograms()):
            self.assertDelta(mtd["calib-2nd"].readY(i), 1.0, 1e-3)
