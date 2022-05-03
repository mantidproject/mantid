# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import sys
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, UseCompatibilityMode,
                                                         AssignSample, TransmissionSample, WavRangeReduction, Set2D)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2D_GDW20_4m_22_02_2D_M3(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set2D()
        MaskFile("USER_SANS2D_212A_2p4_4m_M3_Sans2d_Team_8mm_Changer_After_GuideRealignment.toml")
        AssignSample("SANS2D00068982.nxs")
        TransmissionSample("SANS2D00068976.nxs", "SANS2D00069299.nxs")
        self.returned = WavRangeReduction()

    def validate(self):
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('SpectraMap')

        return self.returned, "SANS2D_GDW20_4m_cycle_22_02_2D_M3_ref.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2D_GDW20_4m_22_02_2D_M4(systemtesting.MantidSystemTest):
    def skipTests(self):
        return sys.platform == 'win32' and 'CONDA_PREFIX' in os.environ

    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set2D()
        MaskFile("USER_SANS2D_212A_2p4_4m_M4_Sans2d_Team_8mm_Changer_After_GuideRealignment.toml")
        AssignSample("SANS2D00068982.nxs")
        TransmissionSample("SANS2D00068976.nxs", "SANS2D00069299.nxs")
        self.returned = WavRangeReduction()

    def validate(self):
        self.tolerance = 7e-5  # Tolerance added to handle conda float rounding differences.
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('SpectraMap')

        return self.returned, "SANS2D_GDW20_4m_cycle_22_02_2D_M4_ref.nxs"
