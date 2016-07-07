import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSState import (SANSStateISIS, SANSState)
from SANS2.State.SANSStateData import (SANSStateDataISIS, SANSStateData)
from SANS2.State.SANSStateMove import (SANSStateMoveLOQ)
from SANS2.Common.SANSConstants import SANSConstants


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

        move = SANSStateMoveLOQ()
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
        try:
            state.data = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_descriptor_validators_work(self):
        # Arrange
        state = SANSStateISIS()

        # We are not setting sample_scatter on the SANSStateDataISIS making it invalid
        data = SANSStateDataISIS()

        # Act + Assert
        try:
            state.data = data
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertFalse(is_valid)

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
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
        # Arrange
        state = SANSStateISIS()

        # Prepare state data
        data = SANSStateDataISIS()
        ws_name_sample = "SANS2D00001234"
        ws_name_can = "SANS2D00001234"
        period = 3

        data.sample_scatter = ws_name_sample
        data.sample_scatter_period = period
        data.can_scatter = ws_name_can
        data.can_scatter_period = period

        state.data = data

        # Prepare the move
        move = SANSStateMoveLOQ()
        test_value = 12.4
        test_name = "test_name"
        move.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        move.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value
        move.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        move.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        move.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        move.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        state.move = move

        # Act
        serialized = state.property_manager

        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        state_2 = SANSStateISIS()
        state_2.property_manager = property_manager

        self.assertTrue(state_2.data.sample_scatter == ws_name_sample)
        self.assertTrue(state_2.data.sample_scatter_period == period)
        self.assertTrue(state_2.data.can_scatter == ws_name_can)
        self.assertTrue(state_2.data.can_scatter_period == period)

        self.assertTrue(state_2.move.detectors[SANSConstants.low_angle_bank].x_translation_correction == test_value)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].y_translation_correction == test_value)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].detector_name == test_name)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].detector_name_short == test_name)


if __name__ == '__main__':
    unittest.main()
