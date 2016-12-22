import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm
from SANS2.State.SANSStateMask import (SANSStateMask, SANSStateMaskISIS)
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager


class SANSStateMaskTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateMaskISIS()
        self.assertTrue(isinstance(state, SANSStateMask))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateMaskISIS()

        # Act + Assert
        state.radius_min = 123.4
        self.assertTrue(state.radius_min == 123.4)

        state.radius_max = 133.5
        self.assertTrue(state.radius_max == 133.5)

        state.mask_files = ["MaskSANS2D.txt", "MaskSANS2D.txt"]
        self.assertTrue(state.mask_files == ["MaskSANS2D.txt", "MaskSANS2D.txt"])

        state.phi_min = 1.0
        self.assertTrue(state.phi_min == 1.0)

        state.phi_max = 2.0
        self.assertTrue(state.phi_max == 2.0)

        state.use_mask_phi_mirror = True
        self.assertTrue(state.use_mask_phi_mirror)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateMaskISIS()

        # Act + Assert
        try:
            state.radius_min = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateMaskISIS()
        state.radius_min = 20.0
        state.radius_max = 10.0

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
        # Arrange
        state = SANSStateMaskISIS()
        state.radius_min = 10.0
        state.radius_max = 20.0

        state.mask_files = ["MaskSANS2D.txt", "MaskSANS2D.txt"]

        state.phi_min = 10.0
        state.phi_max = 20.0
        state.use_mask_phi_mirror = False

        state.beam_stop_arm_width = 10.0
        state.beam_stop_arm_angle = 10.0
        state.beam_stop_arm_pos1 = 10.0
        state.beam_stop_arm_pos2 = 10.0

        state.bin_mask_general_start = [10.0]
        state.bin_mask_general_stop = [10.0]

        # Act
        serialized = state.property_manager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        self.assertTrue(type(serialized) == dict)
        self.assertTrue(type(property_manager) == PropertyManager)
        state_2 = create_deserialized_sans_state_from_property_manager(property_manager)

        self.assertTrue(state_2.radius_min == 10.0)
        self.assertTrue(state_2.radius_max == 20.0)
        self.assertTrue(state_2.phi_min == 10.0)
        self.assertTrue(state_2.mask_files == ["MaskSANS2D.txt", "MaskSANS2D.txt"])
        self.assertFalse(state_2.use_mask_phi_mirror)


if __name__ == '__main__':
    unittest.main()
