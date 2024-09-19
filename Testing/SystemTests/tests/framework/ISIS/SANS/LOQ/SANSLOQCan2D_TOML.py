# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import mantid  # noqa
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import (
    LOQ,
    Set2D,
    Detector,
    MaskFile,
    SetDetectorOffsets,
    Gravity,
    AssignSample,
    AssignCan,
    WavRangeReduction,
    UseCompatibilityMode,
)
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQCan2DTest_TOML(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()
        Set2D()
        Detector("main-detector-bank")
        MaskFile("MASK_094AA.toml")
        # apply some small artificial shift
        SetDetectorOffsets("REAR", -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)
        Gravity(True)

        AssignSample("99630.RAW")  # They file seems to be named wrongly.
        AssignCan("99631.RAW")  # The file seems to be named wrongly.

        WavRangeReduction(None, None, False)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        # when comparing LOQ files you seem to need the following
        self.disableChecking.append("Axes")
        return "99630_main_2D_2.2_10.0", "SANSLOQCan2D.nxs"
