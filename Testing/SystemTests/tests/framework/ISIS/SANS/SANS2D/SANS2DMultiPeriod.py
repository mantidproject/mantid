# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods

# test batch mode with sans2d and selecting a period in batch mode
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from ISISCommandInterface import *
from SANSBatchMode import *
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DMultiPeriodSingle(systemtesting.MantidSystemTest):
    reduced = ""

    def runTest(self):
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

        return mtd[self.reduced][6].name(), "SANS2DBatch.nxs"


class SANS2DMultiPeriodBatch(SANS2DMultiPeriodSingle):
    def runTest(self):
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)

        csv_file = FileFinder.getFullPath("SANS2D_multiPeriodTests.csv")

        BatchReduce(csv_file, "nxs", saveAlgs={})
        self.reduced = "5512_SANS2DBatch"
