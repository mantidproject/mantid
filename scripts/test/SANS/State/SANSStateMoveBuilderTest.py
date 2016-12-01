import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateMoveBuilder import get_move_builder
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument)


class SANSStateMoveBuilderTest(unittest.TestCase):
    def test_that_state_for_loq_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_center_position(value)
        builder.set_HAB_x_translation_correction(value)
        builder.set_LAB_sample_centre_pos1(value)

        # Assert
        state = builder.build()
        self.assertTrue(state.center_position == value)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].x_translation_correction == value)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == "HAB")
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == "main-detector-bank")
        self.assertTrue(state.monitor_names[str(2)] == "monitor2")
        self.assertTrue(len(state.monitor_names) == 2)
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].sample_centre_pos1 == value)

    def test_that_state_for_sans2d_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("SANS2D00022048")
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_HAB_x_translation_correction(value)

        # Assert
        state = builder.build()
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].x_translation_correction == value)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == "front")
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == "rear-detector")
        self.assertTrue(state.monitor_names[str(7)] == "monitor7")
        self.assertTrue(len(state.monitor_names) == 8)

    def test_that_state_for_larmor_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LARMOR00002260")
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_HAB_x_translation_correction(value)

        # Assert
        state = builder.build()
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].x_translation_correction == value)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == "front")
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == "DetectorBench")
        self.assertTrue(state.monitor_names[str(5)] == "monitor5")
        self.assertTrue(len(state.monitor_names) == 10)

if __name__ == '__main__':
    unittest.main()
