import unittest
import mantid
from SANS2.State.SANSStateData import (SANSStateDataISIS, SANSStateData)
from StateTestHelper import (assert_validate_error, assert_raises_nothing)


class SANSStateDataTest(unittest.TestCase):
    @staticmethod
    def _get_data_state(**data_entries):
        state = SANSStateDataISIS()
        data_settings = {"sample_scatter": "test", "sample_transmission": "test",
                         "sample_direct": "test", "can_scatter": "test",
                         "can_transmission": "test", "can_direct": "test"}

        for key, value in data_settings.items():
            if key in data_entries:
                value = data_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def assert_raises_for_bad_value_and_raises_nothing_for_good_value(self, data_entries_bad,
                                                                      data_entries_good):
        # Bad values
        state = SANSStateDataTest._get_data_state(**data_entries_bad)
        assert_validate_error(self, ValueError, state)

        # Good values
        state = SANSStateDataTest._get_data_state(**data_entries_good)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_data_object(self):
        state = SANSStateDataISIS()
        self.assertTrue(isinstance(state, SANSStateData))

    def test_that_raises_when_sample_scatter_is_missing(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value({"sample_scatter": None},
                                                                           {"sample_scatter": "test"})

    def test_that_raises_when_transmission_and_direct_are_inconsistently_specified_for_sample(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value({"sample_transmission": None,
                                                                            "sample_direct": "test",
                                                                            "can_transmission": None,
                                                                            "can_direct": None},
                                                                           {"sample_transmission": "test",
                                                                            "sample_direct": "test",
                                                                            "can_transmission": None,
                                                                            "can_direct": None})

    def test_that_raises_when_transmission_and_direct_are_inconsistently_specified_for_can(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value({"can_transmission": "test",
                                                                            "can_direct": None},
                                                                           {"can_transmission": "test",
                                                                            "can_direct": "test"})

    def test_that_raises_when_transmission_but_not_scatter_was_specified_for_can(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value({"can_scatter": None,
                                                                            "can_transmission": "test",
                                                                            "can_direct": "test"},
                                                                           {"can_scatter": "test",
                                                                            "can_transmission": "test",
                                                                            "can_direct": "test"})


if __name__ == '__main__':
    unittest.main()
