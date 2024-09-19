# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid.api import AnalysisDataService, FileFinder
from sans.command_interface.ISISCommandInterface import (
    SANS2D,
    Set1D,
    Detector,
    MaskFile,
    Gravity,
    UseCompatibilityMode,
    AssignSample,
    WavRangeReduction,
    BatchReduce,
)

# test batch mode with sans2d and selecting a period in batch mode
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DMultiPeriodSingleTest_V2(systemtesting.MantidSystemTest):
    reduced = ""

    def runTest(self):
        pass
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)

        AssignSample("5512")
        self.reduced = WavRangeReduction()

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return AnalysisDataService[self.reduced][6].name(), "SANS2DBatch.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DMultiPeriodBatchTest_V2(SANS2DMultiPeriodSingleTest_V2):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)

        csv_file = FileFinder.getFullPath("SANS2D_multiPeriodTests.csv")
        BatchReduce(csv_file, "nxs", saveAlgs={})
        self.reduced = "5512_SANS2DBatch_rear_1DPhi-45.0_45.0"
