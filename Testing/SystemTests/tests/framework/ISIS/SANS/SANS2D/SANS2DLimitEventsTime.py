# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
from ISISCommandInterface import *
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DLimitEventsTime(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()
        MaskFile("MaskSANS2DReductionGUI_LimitEventsTime.txt")
        AssignSample("22048")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "22048rear_1D_1.5_12.5", "SANSReductionGUI_LimitEventsTime.nxs"
