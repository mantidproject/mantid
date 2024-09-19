# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import ZOOM, MaskFile, Set1D, AssignSample, UseCompatibilityMode, WavRangeReduction
from SANS.sans.common.enums import SANSInstrument
from systemtesting import MantidSystemTest


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMTomlFileTest(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("16812_Userfile.toml")
        Set1D()

        AssignSample("16182")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        return "16182_rear_1D_1.75_16.5", "ZOOM_16182_Userfile.nxs"
