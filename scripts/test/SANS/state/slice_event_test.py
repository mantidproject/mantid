# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.common.enums import SANSFacility, SANSInstrument
from SANS.sans.state.StateObjects.StateData import get_data_builder
from SANS.sans.state.StateObjects.StateSliceEvent import StateSliceEvent, get_slice_event_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateSliceEventTest(unittest.TestCase):
    def test_that_raises_when_only_one_time_is_set(self):
        state = StateSliceEvent()
        state.start_time = [1.0, 2.0]
        with self.assertRaises(ValueError):
            state.validate()
        state.end_time = [2.0, 3.0]

    def test_validate_method_raises_value_error_for_mismatching_start_and_end_time_length(self):
        # Arrange
        state = StateSliceEvent()
        state.start_time = [1.0, 2.0]
        state.end_time = [5.0]

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_validate_method_raises_value_error_for_end_time_smaller_than_start_time(self):
        # Arrange
        state = StateSliceEvent()
        state.start_time = [1.0, 2.0, 4.6]
        state.end_time = [1.1, 2.1, 2.5]

        # Act + Assert
        self.assertRaises(ValueError, state.validate)


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
        builder = get_slice_event_builder(data_info)
        self.assertTrue(builder)

        start_time = [0.1, 1.3]
        end_time = [0.2, 1.6]
        builder.set_start_time(start_time)
        builder.set_end_time(end_time)

        # Assert
        state = builder.build()
        self.assertEqual(len(state.start_time), 2)
        self.assertEqual(state.start_time[0], start_time[0])
        self.assertEqual(state.start_time[1], start_time[1])

        self.assertEqual(len(state.end_time), 2)
        self.assertEqual(state.end_time[0], end_time[0])
        self.assertEqual(state.end_time[1], end_time[1])


if __name__ == "__main__":
    unittest.main()
