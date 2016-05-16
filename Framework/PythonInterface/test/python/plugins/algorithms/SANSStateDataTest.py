import unittest
import mantid
from SANSStateData import (SANSStateDataISIS, SANSStateData)
from SANSStateBase import TypedParameter


class SANSStateDataTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateDataISIS()
        self.assertTrue(isinstance(state, SANSStateData))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateDataISIS()
        ws_name = "SANS2D00001234"
        period = 3

        # Assert
        state.sample_scatter = ws_name
        self.assertTrue(state.sample_scatter == ws_name)
        state.sample_scatter_period = period
        self.assertTrue(state.sample_scatter_period == period)

        state.sample_transmission = ws_name
        self.assertTrue(state.sample_scatter == ws_name)
        state.sample_transmission_period = period
        self.assertTrue(state.sample_transmission_period == period)

        state.sample_direct = ws_name
        self.assertTrue(state.sample_direct == ws_name)
        state.sample_direct_period = period
        self.assertTrue(state.sample_direct_period == period)

        state.can_scatter = ws_name
        self.assertTrue(state.can_scatter == ws_name)
        state.can_scatter_period = period
        self.assertTrue(state.can_scatter_period == period)

        state.can_transmission = ws_name
        self.assertTrue(state.can_scatter == ws_name)
        state.can_transmission_period = period
        self.assertTrue(state.can_transmission_period == period)

        state.can_direct = ws_name
        self.assertTrue(state.can_direct == ws_name)
        state.can_direct_period = period
        self.assertTrue(state.can_direct_period == period)

        try:
            state.validate()
            is_valid = True
        except ValueError, e:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_states_raise_invalid_argument_exception(self):
        pass

    def test_that_property_manager_can_be_generated_from_state_object(self):
        # Arrange
        state = SANSStateDataISIS()
        ws_name_sample = "SANS2D00001234"
        ws_name_can = "SANS2D00001234"
        period = 3

        state.sample_scatter = ws_name_sample
        state.sample_scatter_period = period
        state.can_scatter = ws_name_can
        state.can_scatter_period = period

        # Act
        property_manager = state.property_manager

        # Assert
        state_2 = SANSStateDataISIS()
        state_2.property_manager = property_manager
        

if __name__ == '__main__':
    unittest.main()
