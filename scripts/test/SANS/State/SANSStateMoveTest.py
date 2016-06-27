import unittest
import mantid

from State.SANSStateMove import (SANSStateMoveLOQ,SANSStateMoveSANS2D, SANSStateMoveLARMOR, SANSStateMove)
from Common.SANSConstants import SANSConstants


class SANSStateMoveWorkspaceLOQTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveLOQ()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateMoveLOQ()
        test_value = 12.4
        # Assert
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == 0.0)
        state.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == test_value)

        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == 0.0)
        state.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == test_value)

        self.assertTrue(state.center_position == 317.5 / 1000.)
        state.center_position = test_value
        self.assertTrue(state.center_position == test_value)

        # Name of the detector
        test_name = "test_name"
        state.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == test_name)

        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateMoveLOQ()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.center_position = ["sdf"]

        with self.assertRaises(TypeError):
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateMoveLOQ()
        test_name = "test_name"
        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_property_manager_can_be_generated_from_state_object(self):
        pass


class SANSStateMoveWorkspaceSANS2DTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveSANS2D()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateMoveSANS2D()
        test_value = 12.4
        # Assert
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == 0.0)
        state.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == test_value)

        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == 0.0)
        state.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == test_value)

        # Name of the detector
        test_name = "test_name"
        state.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == test_name)

        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)

        state.lab_detector_x = test_value
        self.assertTrue(state.lab_detector_x == test_value)
        state.lab_detector_z = test_value
        self.assertTrue(state.lab_detector_z == test_value)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateMoveSANS2D()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.lab_detector_x = ["sdf"]

        with self.assertRaises(TypeError):
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateMoveSANS2D()
        test_name = "test_name"
        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_property_manager_can_be_generated_from_state_object(self):
        pass


class SANSStateMoveWorkspaceLARMORTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveLARMOR()
        self.assertTrue(isinstance(state, SANSStateMove))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateMoveLARMOR()
        test_value = 12.4
        # Assert
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == 0.0)
        state.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].x_translation_correction == test_value)

        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == 0.0)
        state.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].y_translation_correction == test_value)

        # Name of the detector
        test_name = "test_name"
        state.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].detector_name_short == test_name)

        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)


        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateMoveLARMOR()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateMoveLARMOR()
        test_name = "test_name"
        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name == test_name)
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].detector_name_short == test_name)

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_property_manager_can_be_generated_from_state_object(self):
        pass


if __name__ == '__main__':
    unittest.main()
