import unittest
import mantid
from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm
from SANS2.State.SANSStateSliceEvent import (SANSStateSliceEvent, SANSStateSliceEventISIS)


class SANSStateSliceEventTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateSliceEventISIS()
        self.assertTrue(isinstance(state, SANSStateSliceEvent))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateSliceEventISIS()

        # Act + Assert
        start_time = [12.3, 123.4, 34345.0]
        state.start_time = start_time
        end_time = [12.5, 200., 40000.0]
        state.end_time = end_time

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateSliceEventISIS()

        # Act + Assert
        try:
            state.start_time = "w234234"
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_invalid_list_values_raise_value_error(self):
        # Arrange
        state = SANSStateSliceEventISIS()

        # Act + Assert
        try:
            state.start_time = ["w234234"]
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_validate_method_raises_value_error_for_mismatching_start_and_end_time_length(self):
        # Arrange
        state = SANSStateSliceEventISIS()
        state.start_time = [1.0, 2.0]
        state.end_time = [5.0]

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_validate_method_raises_value_error_for_non_increasing_time(self):
        # Arrange
        state = SANSStateSliceEventISIS()
        state.start_time = [1.0, 2.0, 1.5]
        state.end_time = [1.1, 2.1, 2.5]

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_validate_method_raises_value_error_for_end_time_smaller_than_start_time(self):
        # Arrange
        state = SANSStateSliceEventISIS()
        state.start_time = [1.0, 2.0, 4.6]
        state.end_time = [1.1, 2.1, 2.5]

        # Act + Assert
        self.assertRaises(ValueError, state.validate)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass

        # Arrange
        state = SANSStateSliceEventISIS()
        start_time = [12.6, 34.6, 2334.7]
        end_time = [22.8, 44.9, 4344.7]
        state.start_time = start_time
        state.end_time = end_time

        # Act
        serialized = state.property_manager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        self.assertTrue(type(serialized) == dict)
        self.assertTrue(type(property_manager) == PropertyManager)
        state_2 = SANSStateSliceEventISIS()
        state_2.property_manager = property_manager

        self.assertTrue(state_2.start_time == start_time)
        self.assertTrue(state_2.end_time == end_time)


if __name__ == '__main__':
    unittest.main()
