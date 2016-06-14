import unittest
import mantid

from State.SANSStateMoveWorkspace import (SANSStateMoveWorkspaceLOQ, SANSStateMoveWorkspace)


class SANSStateMoveWorkspaceLOQTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMoveWorkspaceLOQ()
        self.assertTrue(isinstance(state, SANSStateMoveWorkspace))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateMoveWorkspaceLOQ()
        test_value = 12.4
        # Assert
        self.assertTrue(state.x_translation_correction == 0.0)
        state.x_translation_correction = test_value
        self.assertTrue(state.x_translation_correction == test_value)

        self.assertTrue(state.y_translation_correction == 0.0)
        state.y_translation_correction = test_value
        self.assertTrue(state.y_translation_correction == test_value)

        self.assertTrue(state.z_translation_correction == 0.0)
        state.z_translation_correction = test_value
        self.assertTrue(state.z_translation_correction == test_value)

        self.assertTrue(state.center_position == 317.5 / 1000.)
        state.center_position = test_value
        self.assertTrue(state.center_position == test_value)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateMoveWorkspaceLOQ()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.center_position = ["sdf"]

    def test_that_property_manager_can_be_generated_from_state_object(self):
        pass


if __name__ == '__main__':
    unittest.main()
