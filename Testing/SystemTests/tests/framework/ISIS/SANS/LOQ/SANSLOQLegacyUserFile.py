# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid import FileFinder
from SANS.sans.common.enums import SANSInstrument
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from sans.command_interface.ISISCommandInterface import LOQ, UseCompatibilityMode, BatchReduce
from mantid.simpleapi import GroupWorkspaces


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQLegacyUserFile_M3(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()

        csv_file = FileFinder.getFullPath("toml_legacy_file_m3.csv")
        BatchReduce(csv_file, "raw")

    def validate(self):
        self.tolerance = 1e-4
        names = ["main", "merged"]
        expected = [f"{i}_from_m3_{i}_1D_2.2_10.0" for i in names]
        expected.append("hab_from_m3_HAB_1D_2.2_10.0")
        expected.append("merged_shifted_from_m3_merged_1D_2.2_10.0")
        expected = sorted(expected)

        GroupWorkspaces(InputWorkspaces=expected, OutputWorkspace="grouped")
        return "grouped", "SANS_LOQ_m3_legacy_toml_user_file.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQTomlUserFile_M3(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()

        csv_file = FileFinder.getFullPath("toml_v1_file_m3.csv")
        BatchReduce(csv_file, "raw")

    def validate(self):
        self.tolerance = 1e-4

        names = ["main", "merged"]
        expected = [f"{i}_from_m3_{i}_1D_2.2_10.0" for i in names]
        expected.append("hab_from_m3_HAB_1D_2.2_10.0")
        expected.append("merged_shifted_from_m3_merged_1D_2.2_10.0")
        expected = sorted(expected)

        GroupWorkspaces(InputWorkspaces=expected, OutputWorkspace="grouped")
        return "grouped", "SANS_LOQ_m3_legacy_toml_user_file.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQLegacyUserFile_M4(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()

        csv_file = FileFinder.getFullPath("toml_legacy_file_m4.csv")
        BatchReduce(csv_file, "raw")

    def validate(self):
        self.tolerance = 1e-4
        return "main_from_m4_main_1D_2.2_10.0", "SANS_LOQ_m4_legacy_toml_user_file.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQTomlUserFile_M4(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        LOQ()

        csv_file = FileFinder.getFullPath("toml_v1_file_m4.csv")
        BatchReduce(csv_file, "raw")

    def validate(self):
        self.tolerance = 1e-4
        return "main_from_m4_main_1D_2.2_10.0", "SANS_LOQ_m4_legacy_toml_user_file.nxs"
