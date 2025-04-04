# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import LoadNexus, Plus
from mantid import config, FileFinder
from ISISCommandInterface import Detector, Gravity, LOQ, MaskFile, Set1D
from SANSBatchMode import BatchReduce
import os.path

from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQBatch(systemtesting.MantidSystemTest):
    def runTest(self):
        # DataPath("../Data/LOQ/")
        # UserPath("../Data/LOQ/")

        # here we are testing the LOQ setup
        LOQ()
        # rear detector
        Detector("main-detector-bank")
        # test batch mode, although only the analysis from the last line is checked
        # Find the file , this should really be in the BatchReduce reduction step
        csv_file = FileFinder.getFullPath("batch_input.csv")

        Set1D()
        MaskFile("MASK.094AA")
        Gravity(True)

        BatchReduce(csv_file, "raw", plotresults=False, saveAlgs={"SaveCanSAS1D": "xml", "SaveNexus": "nxs"})

        LoadNexus(Filename="54433sans.nxs", OutputWorkspace="result")
        Plus(LHSWorkspace="result", RHSWorkspace="99630sanotrans", OutputWorkspace="result")

        os.remove(os.path.join(config["defaultsave.directory"], "54433sans.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "99630sanotrans.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "54433sans.xml"))
        os.remove(os.path.join(config["defaultsave.directory"], "99630sanotrans.xml"))

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "result", "SANSLOQBatch.nxs"
