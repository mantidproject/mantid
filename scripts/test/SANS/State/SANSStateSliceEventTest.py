import unittest
import mantid

from SANS2.State.SANSStateSliceEvent import (SANSStateSliceEvent, SANSStateSliceEventISIS)
from StateTestHelper import (assert_validate_error)


class SANSStateSliceEventTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateSliceEventISIS()
        self.assertTrue(isinstance(state, SANSStateSliceEvent))

    def test_that_raises_when_only_one_time_is_set(self):
        state = SANSStateSliceEventISIS()
        state.start_time = [1.0, 2.0]
        assert_validate_error(self, ValueError, state)
        state.end_time = [2.0, 3.0]

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



if __name__ == '__main__':
    unittest.main()
