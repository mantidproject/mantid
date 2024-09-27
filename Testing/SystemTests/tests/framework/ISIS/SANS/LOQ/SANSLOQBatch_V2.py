# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
import os.path

from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.kernel import config
from mantid.api import FileFinder
from mantid.simpleapi import LoadNexus, Plus
from sans_core.command_interface.ISISCommandInterface import LOQ, Detector, Set1D, MaskFile, Gravity, BatchReduce, UseCompatibilityMode
from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQBatchTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()
        Detector("main-detector-bank")
        csv_file = FileFinder.getFullPath("batch_input.csv")

        Set1D()
        MaskFile("MASK.094AA")
        Gravity(True)

        BatchReduce(csv_file, "raw", plotresults=False, saveAlgs={"SaveCanSAS1D": "xml", "SaveNexus": "nxs"})

        LoadNexus(Filename="54433sans_main_1D_2.2_10.0.nxs", OutputWorkspace="result")
        Plus(LHSWorkspace="result", RHSWorkspace="99630sanotrans_main_1D_2.2_10.0", OutputWorkspace="result")

        os.remove(os.path.join(config["defaultsave.directory"], "54433sans_main_1D_2.2_10.0.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "99630sanotrans_main_1D_2.2_10.0.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "54433sans_main_1D_2.2_10.0.xml"))
        os.remove(os.path.join(config["defaultsave.directory"], "99630sanotrans_main_1D_2.2_10.0.xml"))

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "result", "SANSLOQBatch.nxs"
