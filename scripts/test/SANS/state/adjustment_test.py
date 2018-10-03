# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.adjustment import (StateAdjustment, get_adjustment_builder)
from sans.state.data import (get_data_builder)
from sans.state.calculate_transmission import StateCalculateTransmission
from sans.state.normalize_to_monitor import StateNormalizeToMonitor
from sans.state.wavelength_and_pixel_adjustment import StateWavelengthAndPixelAdjustment
from sans.common.enums import (SANSFacility, SANSInstrument, FitType)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class MockStateNormalizeToMonitor(StateNormalizeToMonitor):
    def validate(self):
        pass


class MockStateCalculateTransmission(StateCalculateTransmission):
    def validate(self):
        pass


class MockStateWavelengthAndPixelAdjustment(StateWavelengthAndPixelAdjustment):
    def validate(self):
        pass


class StateReductionTest(unittest.TestCase):
    def test_that_raises_when_calculate_transmission_is_not_set(self):
        state = StateAdjustment()
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        self.assertRaises(ValueError, state.validate)
        state.calculate_transmission = MockStateCalculateTransmission()
        try:
            state.validate()
        except ValueError:
            self.fail()

    def test_that_raises_when_normalize_to_monitor_is_not_set(self):
        state = StateAdjustment()
        state.calculate_transmission = MockStateCalculateTransmission()
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        self.assertRaises(ValueError, state.validate)
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        try:
            state.validate()
        except ValueError:
            self.fail()

    def test_that_raises_when_wavelength_and_pixel_adjustment_is_not_set(self):
        state = StateAdjustment()
        state.calculate_transmission = MockStateCalculateTransmission()
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        self.assertRaises(ValueError, state.validate)
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        try:
            state.validate()
        except ValueError:
            self.fail()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateAdjustmentBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_adjustment_builder(data_info)
        self.assertTrue(builder)

        builder.set_calculate_transmission(MockStateCalculateTransmission())
        builder.set_normalize_to_monitor(MockStateNormalizeToMonitor())
        builder.set_wavelength_and_pixel_adjustment(MockStateWavelengthAndPixelAdjustment())
        builder.set_wide_angle_correction(False)
        builder.set_show_transmission(False)
        state = builder.build()

        # # Assert
        self.assertTrue(not state.wide_angle_correction)
        self.assertTrue(not state.show_transmission)
        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)


if __name__ == '__main__':
    unittest.main()
