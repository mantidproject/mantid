import unittest
import mantid
from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSStateMove import (SANSStateMoveLOQ,SANSStateMoveSANS2D, SANSStateMoveLARMOR, SANSStateMove)
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSConstants import SANSConstants


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

        state.detectors[SANSConstants.high_angle_bank].sample_centre_pos1 = test_value
        self.assertTrue(state.detectors[SANSConstants.high_angle_bank].sample_centre_pos1 == test_value)
        state.detectors[SANSConstants.low_angle_bank].sample_centre_pos2 = test_value
        self.assertTrue(state.detectors[SANSConstants.low_angle_bank].sample_centre_pos2 == test_value)

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
        try:
            state.center_position = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

        try:
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

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
        try:
            state.lab_detector_x = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

        try:
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

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
        try:
            state.detectors[SANSConstants.high_angle_bank].detector_name_short = 123
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

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
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
        # Arrange
        state = SANSStateMoveLARMOR()
        test_value = 12.4
        test_name = "test_name"
        state.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        state.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value

        state.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        state.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name

        # Act
        serialized = state.property_manager

        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        pmgr = fake.getProperty("Args").value

        # Assert
        state_2 = create_deserialized_sans_state_from_property_manager(pmgr)
        state_2.property_manager = pmgr


if __name__ == '__main__':
    unittest.main()
