import unittest
import mantid

from SANS2.State.SANSStateNormalizeToMonitor import (SANSStateNormalizeToMonitor, SANSStateNormalizeToMonitorISIS,
                                                     SANSStateNormalizeToMonitorLOQ)
from SANS2.Common.SANSType import (RebinType, RangeStepType)
from StateTestHelper import assert_validate_error, assert_raises_nothing


class SANSStateNormalizeToMonitorTest(unittest.TestCase):
    @staticmethod
    def _get_normalize_to_monitor_state(**kwargs):
        state = SANSStateNormalizeToMonitorISIS()
        default_entries = {"prompt_peak_correction_min": 12., "prompt_peak_correction_max": 17.,
                           "rebin_type": RebinType.Rebin, "wavelength_low": 1.5, "wavelength_high": 2.7,
                           "wavelength_step": 0.5, "incident_monitor": 1, "wavelength_step_type": RangeStepType.Lin,
                           "background_TOF_general_start": 1.4, "background_TOF_general_stop": 24.5,
                           "background_TOF_monitor_start": {"1": 123, "2": 123},
                           "background_TOF_monitor_stop": {"1": 234, "2": 2323}}

        for key, value in default_entries.items():
            if key in kwargs:
                value = kwargs[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def assert_raises_for_bad_value_and_raises_nothing_for_good_value(self, entry_name, bad_value, good_value):
        kwargs = {entry_name: bad_value}
        state = self._get_normalize_to_monitor_state(**kwargs)
        assert_validate_error(self, ValueError, state)
        setattr(state, entry_name, good_value)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_normalize_to_monitor_object(self):
        state = SANSStateNormalizeToMonitorISIS()
        self.assertTrue(isinstance(state, SANSStateNormalizeToMonitor))

    def test_that_normalize_to_monitor_for_loq_has_default_prompt_peak(self):
        state = SANSStateNormalizeToMonitorLOQ()
        self.assertTrue(state.prompt_peak_correction_max == 20500.)
        self.assertTrue(state.prompt_peak_correction_min == 19000.)

    def test_that_raises_for_partially_set_prompt_peak(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("prompt_peak_correction_min", None, 1.)

    def test_that_raises_for_inconsistent_prompt_peak(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("prompt_peak_correction_max", 1., 30.)

    def test_that_raises_for_missing_incident_monitor(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("incident_monitor", None, 1)

    def test_that_raises_for_partially_set_general_background_tof(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_general_start", None, 1.)

    def test_that_raises_for_inconsistent_general_background_tof(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_general_start", 100., 1.)

    def test_that_raises_for_partially_set_monitor_background_tof(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_monitor_start", None,
                                                                           {"1": 123, "2": 123})

    def test_that_raises_for_monitor_background_tof_with_different_lengths(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_monitor_start", {"1": 123},
                                                                           {"1": 123, "2": 123})

    def test_that_raises_for_monitor_background_tof_with_differing_spectrum_numbers(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_monitor_start",
                                                                           {"1": 123, "5": 123},
                                                                           {"1": 123, "2": 123})

    def test_that_raises_for_monitor_background_tof_with_inconsistent_bounds(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("background_TOF_monitor_start",
                                                                           {"1": 123, "2": 191123},
                                                                           {"1": 123, "2": 123})

if __name__ == '__main__':
    unittest.main()
