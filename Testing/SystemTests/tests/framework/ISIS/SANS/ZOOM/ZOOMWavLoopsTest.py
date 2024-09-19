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
import unittest


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMWavLoopsTest(MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("16812_Userfile_wav_loops.toml")
        Set1D()

        AssignSample("16182")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        return "16182_rear_1D_1.75_16.5", "ZOOM_16182_Userfile.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMWavRangeReductionLoopsTest(MantidSystemTest):
    """This test uses a user file without multiple wavelength ranges, but sets multiple wavelength ranges in the call
    to WavRangeReduction"""

    def runTest(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("16812_Userfile.toml")
        Set1D()

        AssignSample("16182")
        WavRangeReduction([1.75, 6.5, 10.75], [6.5, 10.75, 16.5])

    def validate(self):
        self.disableChecking.append("Instrument")
        return "16182_rear_1D_1.75_16.5", "ZOOM_16182_Userfile.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMWavRangeReductionValidation(unittest.TestCase):
    """These tests check the validation in WavRangeReduction fails as expected"""

    def setUp(self):
        UseCompatibilityMode()
        ZOOM()
        MaskFile("16812_Userfile.toml")
        Set1D()

        AssignSample("16182")

    def test_throws_if_first_input_is_list_and_other_is_not_set(self):
        start = [1.75, 6.5, 10.75]
        end = [6.5, 10.75, 16.5]
        self.assertRaises(RuntimeError, WavRangeReduction, start, end)

    def test_throws_if_second_input_is_list_and_other_is_not_set(self):
        start = None
        end = [6.5, 10.75, 16.5]
        self.assertRaises(RuntimeError, WavRangeReduction, start, end)

    def test_throws_if_first_input_is_list_and_other_is_not(self):
        start = [1.75, 6.5, 10.75]
        end = 16.5
        self.assertRaises(RuntimeError, WavRangeReduction, start, end)

    def test_throws_if_second_input_is_list_and_other_is_not(self):
        start = 1.75
        end = [6.5, 10.75, 16.5]
        self.assertRaises(RuntimeError, WavRangeReduction, start, end)
