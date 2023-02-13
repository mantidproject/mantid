# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import RangeStepType, DetectorType, SANSFacility, SANSInstrument
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import (
    StateWavelengthAndPixelAdjustment,
    get_wavelength_and_pixel_adjustment_builder,
)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentTest(unittest.TestCase):
    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = StateWavelengthAndPixelAdjustment()
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_low = [1.0]
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_high = [2.0]
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_interval.wavelength_step = 2.0
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_step_type = RangeStepType.LIN
        self.assertIsNone(state.validate())

    def test_convert_step_type_from_RANGE_LIN_to_LIN(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_step_type = RangeStepType.RANGE_LIN
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LIN)

    def test_convert_step_type_from_RANGE_LOG_to_LOG(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_step_type = RangeStepType.RANGE_LOG
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LOG)

    def test_convert_step_type_does_not_change_LIN(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_step_type = RangeStepType.LIN
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LIN)

    def test_convert_step_type_does_not_change_LOG(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_step_type = RangeStepType.LOG
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LOG)

    def test_convert_step_type_does_not_change_NOT_SET(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_step_type = RangeStepType.NOT_SET
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.NOT_SET)


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentBuilderTest(unittest.TestCase):
    def test_that_wavelength_and_pixel_adjustment_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_wavelength_and_pixel_adjustment_builder(data_info)
        self.assertTrue(builder)

        builder.set_HAB_pixel_adjustment_file("test")
        builder.set_HAB_wavelength_adjustment_file("test2")

        state = builder.build()

        # Assert
        self.assertTrue(state.adjustment_files[DetectorType.HAB.value].pixel_adjustment_file == "test")
        self.assertTrue(state.adjustment_files[DetectorType.HAB.value].wavelength_adjustment_file == "test2")


if __name__ == "__main__":
    unittest.main()
