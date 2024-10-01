# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_sans_system_test import ISISSansSystemTest

from mantid.simpleapi import GroupWorkspaces
from sans.command_interface.ISISCommandInterface import UseCompatibilityMode, ZOOM, MaskFile, BatchReduce
from sans.common.enums import SANSInstrument
from systemtesting import MantidSystemTest


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMLegacyUserFileTest_m4(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("USER_ZOOM_Bergstrom_4m_SampleChanger_202A_8mm_Small_BEAMSTOP_skinny.txt")

        BatchReduce("4m_legacy_toml.csv", "raw")

    def validate(self):
        self.disableChecking.append("Instrument")
        GroupWorkspaces(["17034_rear_1D_1.75_16.5", "17035_rear_1D_1.75_16.5"], OutputWorkspace="grouped")
        return "grouped", "SANSZOOMTomlFileConv_m4.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMV1UserFileTest_m4(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("USER_ZOOM_Bergstrom_4m_SampleChanger_202A_8mm_Small_BEAMSTOP_skinny.toml")

        BatchReduce("4m_legacy_toml.csv", "raw")

    def validate(self):
        self.disableChecking.append("Instrument")
        GroupWorkspaces(["17034_rear_1D_1.75_16.5", "17035_rear_1D_1.75_16.5"], OutputWorkspace="grouped")
        return "grouped", "SANSZOOMTomlFileConv_m4.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMLegacyUserFileTest_m8(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("USER_ZOOM_Bergstrom_8m_samplechanger_smallBS_203A_8mm.txt")

        BatchReduce("8m_legacy_toml.csv", "raw")

    def validate(self):
        self.disableChecking.append("Instrument")
        GroupWorkspaces(["17106_rear_1D_1.75_15.0", "17108_rear_1D_1.75_15.0"], OutputWorkspace="grouped")
        return "grouped", "SANSZOOMTomlFileConv_m8.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMV1UserFileTest_m8(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("USER_ZOOM_Bergstrom_8m_samplechanger_smallBS_203A_8mm.toml")

        BatchReduce("8m_legacy_toml.csv", "raw")

    def validate(self):
        self.disableChecking.append("Instrument")
        GroupWorkspaces(["17106_rear_1D_1.75_15.0", "17108_rear_1D_1.75_15.0"], OutputWorkspace="grouped")
        return "grouped", "SANSZOOMTomlFileConv_m8.nxs"
