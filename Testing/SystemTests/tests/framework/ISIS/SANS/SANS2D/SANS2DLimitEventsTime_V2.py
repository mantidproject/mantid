# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
import mantid  # noqa
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import SANS2D, MaskFile, AssignSample, WavRangeReduction, UseCompatibilityMode
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DLimitEventsTimeTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile("MaskSANS2DReductionGUI_LimitEventsTime.txt")
        AssignSample("22048")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "22048_rear_1D_1.5_12.5", "SANSReductionGUI_LimitEventsTime.nxs"
