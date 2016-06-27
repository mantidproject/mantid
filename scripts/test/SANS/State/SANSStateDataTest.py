import unittest
import mantid
from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm
from State.SANSStateData import (SANSStateDataISIS, SANSStateData)


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

        state.calibration = ws_name
        self.assertTrue(state.calibration == ws_name)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateDataISIS()

        # Act + Assert
        with self.assertRaises(TypeError):
            state.sample_scatter = ["sdf"]
        with self.assertRaises(TypeError):
            state.sample_transmission = ["sdf"]
        with self.assertRaises(TypeError):
            state.sample_direct = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_scatter = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_transmission = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_direct = ["sdf"]
        with self.assertRaises(TypeError):
            state.calibration = ["sdf"]

        with self.assertRaises(TypeError):
            state.sample_scatter_period = ["sdf"]
        with self.assertRaises(TypeError):
            state.sample_transmission_period = ["sdf"]
        with self.assertRaises(TypeError):
            state.sample_direct_period = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_scatter_period = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_transmission_period = ["sdf"]
        with self.assertRaises(TypeError):
            state.can_direct_period = ["sdf"]

    def test_that_descriptor_validators_work(self):
        # Arrange
        state = SANSStateDataISIS()

        # Act + Assert
        with self.assertRaises(ValueError):
            state.sample_scatter_period = -1
        with self.assertRaises(ValueError):
            state.sample_transmission_period = -1
        with self.assertRaises(ValueError):
            state.sample_direct_period = -1
        with self.assertRaises(ValueError):
            state.can_scatter_period = -1
        with self.assertRaises(ValueError):
            state.can_transmission_period = -1
        with self.assertRaises(ValueError):
            state.can_direct_period = -1

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateDataISIS()
        state.sample_scatter = "sample_scat"
        state.sample_transmission = "sample_trans"

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
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
        serialized = state.property_manager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        pmgr = fake.getProperty("Args").value

        # Assert
        self.assertTrue(type(serialized) == dict)
        self.assertTrue(type(pmgr) == PropertyManager)
        state_2 = SANSStateDataISIS()
        state_2.property_manager = pmgr
        self.assertTrue(state_2.sample_scatter == ws_name_sample)
        self.assertTrue(state_2.sample_scatter_period == period)
        self.assertTrue(state_2.can_scatter == ws_name_can)
        self.assertTrue(state_2.can_scatter_period == period)


if __name__ == '__main__':
    unittest.main()
