# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import PelicanCrystalProcessing


class PelicanCrystalProcessingSingleTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        saveFolder = config["defaultsave.directory"]
        PelicanCrystalProcessing(
            "44464",
            EnergyTransfer="-2,0.05,2",
            MomentumTransfer="0,0.05,2",
            OutputFolder=saveFolder,
            ConfigurationFile="pelican_doctest.ini",
            KeepReducedWorkspace=True,
        )

        spath = os.path.join(saveFolder, "run_44464.nxspe")
        self.assertTrue(os.path.isfile(spath), "Output file not in output folder")

    def validate(self):
        self.disableChecking.append("Instrument")
        return "nxspe_spe_2D", "PelicanReductionExampleNXSPE.nxs"


class PelicanCrystalProcessingScratchTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        saveFolder = config["defaultsave.directory"]
        PelicanCrystalProcessing(
            "44464",
            EnergyTransfer="-2,0.05,2",
            MomentumTransfer="0,0.05,2",
            OutputFolder=saveFolder,
            ScratchFolder=saveFolder,
            ConfigurationFile="pelican_doctest.ini",
        )

        spath = os.path.join(saveFolder, "run_44464.nxspe")
        self.assertTrue(os.path.isfile(spath), "Output file not in output folder")

        # confirm temp files and temp workspace are removed
        self.assertTrue("test_spe_2D" not in mtd, "Temp workspace not removed")
        for sfile in ["PLN0044464_sample.nxs", "PLN0044464.nxs"]:
            tpath = os.path.join(saveFolder, sfile)
            self.assertTrue(not os.path.isfile(tpath), f"Temp file {sfile} was not removed")

    def validate(self):
        return True
