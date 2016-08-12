import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSStateSave import (SANSStateSave, SANSStateSaveISIS)
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSEnumerations import SaveType


class SANSStateReductionTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateSaveISIS()
        self.assertTrue(isinstance(state, SANSStateSave))

    def test_that_reduction_state_gets_and_sets(self):
        # Arrange
        state = SANSStateSaveISIS()

        # Act
        state.file_name = "test_file_name"
        state.zero_free_correction = True
        state.file_format = [SaveType.Nexus, SaveType.CanSAS]

        # Assert
        self.assertTrue(state.file_name == "test_file_name")
        self.assertTrue(state.zero_free_correction)
        self.assertTrue(state.file_format == [SaveType.Nexus, SaveType.CanSAS])

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateSaveISIS()

        # Act and Assert
        try:
            state.file_format = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass

        # Arrange
        state = SANSStateSaveISIS()
        state.file_name = "test_file_name"
        state.zero_free_correction = True
        state.file_format = [SaveType.Nexus, SaveType.CanSAS]

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
        state_2.property_manager = property_manager

        self.assertTrue(state_2.file_name == "test_file_name")
        self.assertTrue(state_2.zero_free_correction)
        self.assertTrue(state_2.file_format == [SaveType.Nexus, SaveType.CanSAS])


if __name__ == '__main__':
    unittest.main()
