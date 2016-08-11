import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm
from SANS2.State.SANSStateWavelength import (SANSStateWavelength, SANSStateWavelengthISIS)
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSEnumerations import (RebinType, RangeStepType)


class SANSStateWavelengthTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateWavelengthISIS()
        self.assertTrue(isinstance(state, SANSStateWavelength))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateWavelengthISIS()

        # Act + Assert
        state.wavelength_low = 123.4
        self.assertTrue(state.wavelength_low == 123.4)

        state.wavelength_high = 133.5
        self.assertTrue(state.wavelength_high == 133.5)

        state.wavelength_step = 1.2
        self.assertTrue(state.wavelength_step == 1.2)

        state.rebin_type = RebinType.Rebin
        self.assertTrue(state.rebin_type is RebinType.Rebin)

        state.wavelength_step_type = RangeStepType.Lin
        self.assertTrue(state.wavelength_step_type is RangeStepType.Lin)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateWavelengthISIS()

        # Act + Assert
        try:
            state.wavelength_step_type = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_validate_method_raises_value_error_for_invalid_state(self):
        # Arrange
        state = SANSStateWavelengthISIS()
        state.wavelength_low = 20.0
        state.wavelength_high = 10.0

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
        # Arrange
        state = SANSStateWavelengthISIS()
        state.wavelength_low = 10.0
        state.wavelength_high = 20.0
        state.wavelength_step = 2.0
        state.wavelength_step_type = RangeStepType.Lin
        state.rebin_type = RebinType.Rebin

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

        self.assertTrue(state_2.wavelength_low == 10.0)
        self.assertTrue(state_2.wavelength_high == 20.0)
        self.assertTrue(state_2.wavelength_step == 2.0)
        self.assertTrue(state_2.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(state_2.rebin_type is RebinType.Rebin)


if __name__ == '__main__':
    unittest.main()
