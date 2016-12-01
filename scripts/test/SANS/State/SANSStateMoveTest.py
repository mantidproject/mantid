import unittest
import mantid

from SANS2.State.SANSStateMove import (SANSStateMoveLOQ,SANSStateMoveSANS2D, SANSStateMoveLARMOR, SANSStateMoveISIS,
                                       SANSStateMove)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSType import CanonicalCoordinates
from StateTestHelper import assert_validate_error, assert_raises_nothing


class SANSStateMoveWorkspaceTest(unittest.TestCase):
    def test_that_raises_if_the_detector_name_is_not_set_up(self):
        state = SANSStateMoveISIS()
        state.detectors[SANSConstants.low_angle_bank].detector_name = "test"
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = "test"
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = "test"
        assert_validate_error(self, ValueError, state)
        state.detectors[SANSConstants.high_angle_bank].detector_name = "test"
        assert_raises_nothing(self, state)

    def test_that_raises_if_the_short_detector_name_is_not_set_up(self):
        state = SANSStateMoveISIS()
        state.detectors[SANSConstants.high_angle_bank].detector_name = "test"
        state.detectors[SANSConstants.low_angle_bank].detector_name = "test"
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = "test"
        assert_validate_error(self, ValueError, state)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = "test"
        assert_raises_nothing(self, state)

    def test_that_general_isis_default_values_are_set_up(self):
        state = SANSStateMoveISIS()
        self.assertTrue(state.sample_offset == 0.0)
        self.assertTrue(state.sample_offset_direction is CanonicalCoordinates.Z)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].x_translation_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].z_translation_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].rotation_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].side_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].radius_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].x_tilt_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_tilt_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].z_tilt_correction == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].sample_centre_pos1 == 0.0)
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].sample_centre_pos2 == 0.0)


class SANSStateMoveWorkspaceLOQTest(unittest.TestCase):
    def test_that_is_sans_state_move_object(self):
        state = SANSStateMoveLOQ()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_LOQ_has_centre_positon_set_up(self):
        state = SANSStateMoveLOQ()
        self.assertTrue(state.center_position == 317.5 / 1000.)
        self.assertTrue(state.monitor_names == {})


class SANSStateMoveWorkspaceSANS2DTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveSANS2D()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_sans2d_has_default_values_set_up(self):
        # Arrange
        state = SANSStateMoveSANS2D()
        self.assertTrue(state.hab_detector_radius == 306.0/1000.)
        self.assertTrue(state.hab_detector_default_sd_m == 4.0)
        self.assertTrue(state.hab_detector_default_x_m == 1.1)
        self.assertTrue(state.lab_detector_default_sd_m == 4.0)
        self.assertTrue(state.hab_detector_x == 0.0)
        self.assertTrue(state.hab_detector_z == 0.0)
        self.assertTrue(state.hab_detector_rotation == 0.0)
        self.assertTrue(state.lab_detector_x == 0.0)
        self.assertTrue(state.lab_detector_z == 0.0)
        self.assertTrue(state.monitor_names == {})
        self.assertTrue(state.monitor_4_offset == 0.0)


class SANSStateMoveWorkspaceLARMORTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveLARMOR()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_can_set_and_get_values(self):
        state = SANSStateMoveLARMOR()
        self.assertTrue(state.bench_rotation == 0.0)
        self.assertTrue(state.monitor_names == {})


if __name__ == '__main__':
    unittest.main()
