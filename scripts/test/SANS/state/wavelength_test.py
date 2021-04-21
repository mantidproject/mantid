# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import (SANSFacility, SANSInstrument, RebinType, RangeStepType)
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateWavelength import (StateWavelength, get_wavelength_builder)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthTest(unittest.TestCase):

    def test_that_is_sans_state_data_object(self):
        state = StateWavelength()
        self.assertTrue(isinstance(state, StateWavelength))

    def test_convert_step_type_from_RANGE_LIN_to_LIN(self):
        state = StateWavelength()
        state.wavelength_step_type = RangeStepType.RANGE_LIN
        self.assertEqual(state.wavelength_step_type_lin_log,  RangeStepType.LIN)

    def test_convert_step_type_from_RANGE_LOG_to_LOG(self):
        state = StateWavelength()
        state.wavelength_step_type = RangeStepType.RANGE_LOG
        self.assertEqual(state.wavelength_step_type_lin_log,  RangeStepType.LOG)

    def test_convert_step_type_does_not_change_LIN(self):
        state = StateWavelength()
        state.wavelength_step_type = RangeStepType.LIN
        self.assertEqual(state.wavelength_step_type_lin_log,  RangeStepType.LIN)

    def test_convert_step_type_does_not_change_LOG(self):
        state = StateWavelength()
        state.wavelength_step_type = RangeStepType.LOG
        self.assertEqual(state.wavelength_step_type_lin_log,  RangeStepType.LOG)

    def test_convert_step_type_does_not_change_NOT_SET(self):
        state = StateWavelength()
        state.wavelength_step_type = RangeStepType.NOT_SET
        self.assertEqual(state.wavelength_step_type_lin_log,  RangeStepType.NOT_SET)


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------

class StateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_wavelength_builder(data_info)
        self.assertTrue(builder)

        builder.state.wavelength_interval.wavelength_full_range = (10.0, 20.0)
        builder.state.wavelength_interval.wavelength_step = 3.0
        builder.set_wavelength_step_type(RangeStepType.LIN)
        builder.set_rebin_type(RebinType.REBIN)

        # Assert
        state = builder.build()

        self.assertEqual(state.wavelength_interval.wavelength_full_range,  (10.0, 20.0))
        self.assertEqual(state.wavelength_step_type, RangeStepType.LIN)
        self.assertEqual(state.rebin_type, RebinType.REBIN)


if __name__ == '__main__':
    unittest.main()
