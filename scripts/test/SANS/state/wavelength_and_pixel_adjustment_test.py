# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.wavelength_and_pixel_adjustment import (StateWavelengthAndPixelAdjustment,
                                                        get_wavelength_and_pixel_adjustment_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (RebinType, RangeStepType, DetectorType, SANSFacility, SANSInstrument)
from state_test_helper import assert_validate_error, assert_raises_nothing
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentTest(unittest.TestCase):
    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = StateWavelengthAndPixelAdjustment()
        assert_validate_error(self, ValueError, state)
        state.wavelength_low = [1.]
        assert_validate_error(self, ValueError, state)
        state.wavelength_high = [2.]
        assert_validate_error(self, ValueError, state)
        state.wavelength_step = 2.
        assert_validate_error(self, ValueError, state)
        state.wavelength_step_type = RangeStepType.Lin
        assert_raises_nothing(self, state)

    def test_that_raises_when_lower_wavelength_is_smaller_than_high_wavelength(self):
        state = StateWavelengthAndPixelAdjustment()
        state.wavelength_low = [2.]
        state.wavelength_high = [1.]
        state.wavelength_step = 2.
        state.wavelength_step_type = RangeStepType.Lin
        assert_validate_error(self, ValueError, state)


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
        builder.set_wavelength_low([1.5])
        builder.set_wavelength_high([2.7])
        builder.set_wavelength_step(0.5)
        builder.set_wavelength_step_type(RangeStepType.Lin)

        state = builder.build()

        # Assert
        self.assertTrue(state.adjustment_files[DetectorType.HAB.name].pixel_adjustment_file == "test")
        self.assertTrue(state.adjustment_files[DetectorType.HAB.name].wavelength_adjustment_file == "test2")
        self.assertEqual(state.wavelength_low,  [1.5])
        self.assertEqual(state.wavelength_high,  [2.7])
        self.assertEqual(state.wavelength_step,  0.5)
        self.assertEqual(state.wavelength_step_type, RangeStepType.Lin)


if __name__ == '__main__':
    unittest.main()
