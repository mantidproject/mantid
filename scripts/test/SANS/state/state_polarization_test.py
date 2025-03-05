# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.state.StateObjects.StatePolarization import StateField, StateComponent, StateFilter, StatePolarization


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StatePolarizationTest(unittest.TestCase):
    # ------------------------------------------------------------------------------------------------------------------
    # StateField
    # ------------------------------------------------------------------------------------------------------------------
    def generate_field(self, log: None | str = None, a: None | int = None, p: None | int = None, d: None | int = None) -> StateField:
        state = StateField()
        state.sample_direction_log = log
        state.sample_direction_a = a
        state.sample_direction_p = p
        state.sample_direction_d = d
        return state

    def test_error_when_field_directions_and_log_set(self):
        state_field = self.generate_field("alogfile.nxs", 1, 2, 3)
        with self.assertRaisesRegex(
            ValueError, 'StateField: Provided inputs are illegal. Please see: {"Too many sample direction parameters.":.*'
        ):
            state_field.validate()

    def test_error_when_not_all_directions_set(self):
        state_field = self.generate_field(a=1, d=3)
        with self.assertRaisesRegex(
            ValueError, 'StateField: Provided inputs are illegal. Please see: {"Missing field sample direction parameters.":.*'
        ):
            state_field.validate()

    def test_valid_when_only_log_set(self):
        state_field = self.generate_field("alogfile.txt")
        state_field.validate()

    def test_valid_when_only_directions_set(self):
        state_field = self.generate_field(a=1, p=2, d=4)
        state_field.validate()

    # ------------------------------------------------------------------------------------------------------------------
    # StatePolarization
    # ------------------------------------------------------------------------------------------------------------------
    def test_all_substates_are_validated(self):
        # Created as mocks to avoid inadvertently testing the substates as well.
        flipper_a = mock.create_autospec(StateComponent)
        flipper_b = mock.create_autospec(StateComponent)
        analyser = mock.create_autospec(StateFilter)
        polarizer = mock.create_autospec(StateFilter)
        mag_field = mock.create_autospec(StateField)
        elec_field = mock.create_autospec(StateField)

        state = StatePolarization()
        state.flippers.extend([flipper_a, flipper_b])
        state.analyzer = analyser
        state.polarizer = polarizer
        state.magnetic_field = mag_field
        state.electric_field = elec_field

        state.validate()
        for substate in [flipper_a, flipper_b, analyser, polarizer, mag_field, elec_field]:
            self.assertEqual(substate.validate.call_count, 1)


if __name__ == "__main__":
    unittest.main()
