# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.normalize_to_monitor import (StateNormalizeToMonitor, StateNormalizeToMonitorLOQ,
                                             get_normalize_to_monitor_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (RebinType, RangeStepType, SANSFacility, SANSInstrument)
from state_test_helper import assert_validate_error, assert_raises_nothing
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateNormalizeToMonitorTest(unittest.TestCase):
    @staticmethod
    def _get_normalize_to_monitor_state(**kwargs):
        state = StateNormalizeToMonitor()
        default_entries = {"prompt_peak_correction_min": 12., "prompt_peak_correction_max": 17.,
                           "rebin_type": RebinType.Rebin, "wavelength_low": [1.5], "wavelength_high": [2.7],
                           "wavelength_step": 0.5, "incident_monitor": 1, "wavelength_step_type": RangeStepType.Lin,
                           "background_TOF_general_start": 1.4, "background_TOF_general_stop": 24.5,
                           "background_TOF_monitor_start": {"1": 123, "2": 123},
                           "background_TOF_monitor_stop": {"1": 234, "2": 2323}}

        for key, value in list(default_entries.items()):
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
        state = StateNormalizeToMonitorLOQ()
        self.assertTrue(isinstance(state, StateNormalizeToMonitor))

    def test_that_normalize_to_monitor_for_loq_has_default_prompt_peak(self):
        state = StateNormalizeToMonitorLOQ()
        self.assertEqual(state.prompt_peak_correction_max,  20500.)
        self.assertEqual(state.prompt_peak_correction_min,  19000.)

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


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_normalize_to_monitor_builder(data_info)
        self.assertTrue(builder)

        builder.set_prompt_peak_correction_min(12.0)
        builder.set_prompt_peak_correction_max(17.0)
        builder.set_rebin_type(RebinType.Rebin)
        builder.set_wavelength_low([1.5])
        builder.set_wavelength_high([2.7])
        builder.set_wavelength_step(0.5)
        builder.set_wavelength_step_type(RangeStepType.Lin)
        builder.set_incident_monitor(1)
        builder.set_background_TOF_general_start(1.4)
        builder.set_background_TOF_general_stop(34.4)
        builder.set_background_TOF_monitor_start({"1": 123, "2": 123})
        builder.set_background_TOF_monitor_stop({"1": 234, "2": 2323})

        state = builder.build()

        # Assert
        self.assertEqual(state.prompt_peak_correction_min,  12.0)
        self.assertEqual(state.prompt_peak_correction_max,  17.0)
        self.assertEqual(state.rebin_type, RebinType.Rebin)
        self.assertEqual(state.wavelength_low,  [1.5])
        self.assertEqual(state.wavelength_high,  [2.7])
        self.assertEqual(state.wavelength_step,  0.5)
        self.assertEqual(state.wavelength_step_type, RangeStepType.Lin)
        self.assertEqual(state.background_TOF_general_start,  1.4)
        self.assertEqual(state.background_TOF_general_stop,  34.4)
        self.assertEqual(len(set(state.background_TOF_monitor_start.items()) & set({"1": 123, "2": 123}.items())), 2)
        self.assertEqual(len(set(state.background_TOF_monitor_stop.items()) & set({"1": 234, "2": 2323}.items())), 2)
        self.assertEqual(state.incident_monitor,  1)


if __name__ == '__main__':
    unittest.main()
