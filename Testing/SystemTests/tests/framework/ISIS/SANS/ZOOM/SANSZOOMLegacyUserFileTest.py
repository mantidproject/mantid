# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_sans_system_test import ISISSansSystemTest
from sans_core.command_interface.ISISCommandInterface import UseCompatibilityMode, ZOOM, MaskFile, Set1D, AssignSample, WavRangeReduction
from sans_core.common.enums import SANSInstrument
from systemtesting import MantidSystemTest


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMLegacyUserFileTest(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("USER_ZOOM_SANSteam_4m_SampleChanger_202A_12mm_Large_BEAMSTOP.txt")
        Set1D()

        AssignSample("16182")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        return "16182_rear_1D_1.75_16.5", "ZOOM_16182_Userfile.nxs"
