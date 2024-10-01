# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from sans.command_interface.ISISCommandInterface import (
    SANS2D,
    Set1D,
    MaskFile,
    UseCompatibilityMode,
    AssignSample,
    TransmissionSample,
    WavRangeReduction,
    Set2D,
)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2D_GDW20_12m_22_02_1D(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        MaskFile("USER_SANS2D_212B_5_12m_M3_Sans2d_Team_8mm_Changer_After_GuideRealignment.toml")
        AssignSample("SANS2D00068982.nxs")
        TransmissionSample("SANS2D00068989.nxs", "SANS2D00069299.nxs")
        self.returned = WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("SpectraMap")

        return self.returned, "SANS2D_GDW20_12m_cycle_22_02_1D_ref.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2D_GDW20_12m_22_02_2D(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set2D()
        MaskFile("USER_SANS2D_212B_5_12m_M3_Sans2d_Team_8mm_Changer_After_GuideRealignment.toml")
        AssignSample("SANS2D00068982.nxs")
        TransmissionSample("SANS2D00068989.nxs", "SANS2D00069299.nxs")
        self.returned = WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("SpectraMap")

        return self.returned, "SANS2D_GDW20_12m_cycle_22_02_2D_ref.nxs"
