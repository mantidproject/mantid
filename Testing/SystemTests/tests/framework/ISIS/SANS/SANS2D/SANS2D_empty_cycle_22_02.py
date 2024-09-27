# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from sans_core.command_interface.ISISCommandInterface import (
    SANS2D,
    Set1D,
    MaskFile,
    UseCompatibilityMode,
    AssignSample,
    TransmissionSample,
    WavRangeReduction,
)
from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2D_empty_22_02_1D_M3(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        MaskFile("USER_SANS2D_212A_2p4_4m_M3_Sans2d_Team_8mm_Changer_After_GuideRealignment.toml")
        AssignSample("SANS2D00068981.nxs")
        TransmissionSample("SANS2D00068975.nxs", "SANS2D00069299.nxs")
        self.returned = WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("SpectraMap")

        return self.returned, "SANS2D_empty_cycle_22_02_1D_M3_ref.nxs"
