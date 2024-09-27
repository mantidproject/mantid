# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans_core.common.enums import SANSFacility, SANSInstrument
from sans_core.state.StateObjects.StateData import StateData, get_data_builder
from sans_core.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State test
# ----------------------------------------------------------------------------------------------------------------------
class StateDataTest(unittest.TestCase):
    @staticmethod
    def _get_data_state(**data_entries):
        state = StateData()
        data_settings = {
            "sample_scatter": "test",
            "sample_transmission": "test",
            "sample_direct": "test",
            "can_scatter": "test",
            "can_transmission": "test",
            "can_direct": "test",
        }

        for key, value in list(data_settings.items()):
            if key in data_entries:
                value = data_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def assert_raises_for_bad_value_and_raises_nothing_for_good_value(self, data_entries_bad, data_entries_good):
        # Bad values
        state = StateDataTest._get_data_state(**data_entries_bad)
        with self.assertRaises(ValueError):
            state.validate()

        # Good values
        state = StateDataTest._get_data_state(**data_entries_good)
        self.assertIsNone(state.validate())

    def test_that_raises_when_sample_scatter_is_missing(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value({"sample_scatter": None}, {"sample_scatter": "test"})

    def test_that_raises_when_transmission_and_direct_are_inconsistently_specified_for_sample(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value(
            {"sample_transmission": None, "sample_direct": "test", "can_transmission": None, "can_direct": None},
            {"sample_transmission": "test", "sample_direct": "test", "can_transmission": None, "can_direct": None},
        )

    def test_that_raises_when_transmission_and_direct_are_inconsistently_specified_for_can(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value(
            {"can_transmission": "test", "can_direct": None}, {"can_transmission": "test", "can_direct": "test"}
        )

    def test_that_raises_when_transmission_but_not_scatter_was_specified_for_can(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value(
            {"can_scatter": None, "can_transmission": "test", "can_direct": "test"},
            {"can_scatter": "test", "can_transmission": "test", "can_direct": "test"},
        )


# ----------------------------------------------------------------------------------------------------------------------
# Builder test
# ----------------------------------------------------------------------------------------------------------------------
class StateDataBuilderTest(unittest.TestCase):
    def test_that_data_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(run_number=74044, file_name="LOQ74044")
        # Act
        data_builder = get_data_builder(facility, file_information)

        data_builder.set_sample_scatter("LOQ74044")
        data_builder.set_sample_scatter_period(3)
        data_state = data_builder.build()

        # # Assert
        self.assertEqual(data_state.sample_scatter, "LOQ74044")
        self.assertEqual(data_state.sample_scatter_period, 3)
        self.assertEqual(data_state.sample_direct_period, 0)
        self.assertEqual(data_state.instrument, SANSInstrument.LOQ)
        self.assertEqual(data_state.sample_scatter_run_number, 74044)


if __name__ == "__main__":
    unittest.main()
