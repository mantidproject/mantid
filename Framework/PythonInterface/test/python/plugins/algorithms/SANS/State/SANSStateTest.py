import unittest
import mantid

from State.SANSState import (SANSStateISIS, SANSState)
from State.SANSStateData import (SANSStateDataISIS, SANSStateData)
from State.SANSStateMoveWorkspace import (SANSStateMoveWorkspaceLOQ)
from Common.SANSConstants import SANSConstants

class SANSStateTest(unittest.TestCase):
    def test_that_is_sans_state_object(self):
        state = SANSStateISIS()
        self.assertTrue(isinstance(state, SANSState))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateISIS()

        # Add the different descriptors of the SANSState here:
        data = SANSStateDataISIS()
        data.sample_scatter = "sample_scat"
        state.data = data

        move = SANSStateMoveWorkspaceLOQ()
        move.detectors[SANSConstants.high_angle_bank].detector_name = "test"
        move.detectors[SANSConstants.high_angle_bank].detector_name_short = "test"
        move.detectors[SANSConstants.low_angle_bank].detector_name = "test"
        move.detectors[SANSConstants.low_angle_bank].detector_name_short = "test"
        state.move = move

        # Assert
        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateISIS()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.data = ["sdf"]

    def test_that_descriptor_validators_work(self):
        # Arrange
        state = SANSStateISIS()

        # We are not setting sample_scatter on the SANSStateDataISIS making it invalid
        data = SANSStateDataISIS()

        # Act + Assert
        with self.assertRaises(ValueError):
            state.data = data

    def test_that_sans_state_holds_a_copy_of_the_substates_and_not_only_a_reference(self):
        # Arrange
        state = SANSStateISIS()
        data = SANSStateDataISIS()
        ws_name_1 = "sample_scat"
        ws_name_2 = "sample_scat2"
        data.sample_scatter = ws_name_1
        state.data = data

        # Act
        data.sample_scatter = ws_name_2

        # Assert
        stored_name = state.data.sample_scatter
        self.assertTrue(stored_name == ws_name_1)

    def test_that_property_manager_can_be_generated_from_state_object(self):
        # Arrange
        state = SANSStateISIS()

        data = SANSStateDataISIS()
        ws_name_sample = "SANS2D00001234"
        ws_name_can = "SANS2D00001234"
        period = 3

        data.sample_scatter = ws_name_sample
        data.sample_scatter_period = period
        data.can_scatter = ws_name_can
        data.can_scatter_period = period

        state.data = data

        # Act
        property_manager = state.property_manager

        # Assert
        # state_2 = SANSStateISIS()
        # state_2.property_manager = property_manager
        # self.assertTrue(state_2.data.sample_scatter == ws_name_sample)
        # self.assertTrue(state_2.data.sample_scatter_period == period)
        # self.assertTrue(state_2.data.can_scatter == ws_name_can)
        # self.assertTrue(state_2.data.can_scatter_period == period)


if __name__ == '__main__':
    unittest.main()
