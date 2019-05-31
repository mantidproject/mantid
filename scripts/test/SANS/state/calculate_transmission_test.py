# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.calculate_transmission import (StateCalculateTransmission, StateCalculateTransmissionLOQ,
                                               get_calculate_transmission_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (RebinType, RangeStepType, FitType, DataType, SANSFacility, SANSInstrument)
from state_test_helper import assert_validate_error, assert_raises_nothing
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateCalculateTransmissionTest(unittest.TestCase):
    @staticmethod
    def _set_fit(state, default_settings, custom_settings, fit_key):
        fit = state.fit[fit_key]
        for key, value in list(default_settings.items()):
            if key in custom_settings:
                value = custom_settings[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(fit, key, value)
        state.fit[fit_key] = fit

    @staticmethod
    def _get_calculate_transmission_state(trans_entries, fit_entries):
        state = StateCalculateTransmission()
        if trans_entries is None:
            trans_entries = {}
        trans_settings = {"transmission_radius_on_detector": 12., "transmission_roi_files": ["test.xml"],
                          "transmission_mask_files": ["test.xml"], "default_transmission_monitor": 3,
                          "transmission_monitor": 4, "default_incident_monitor": 1, "incident_monitor": 2,
                          "prompt_peak_correction_min": 123., "prompt_peak_correction_max": 1234.,
                          "rebin_type": RebinType.Rebin, "wavelength_low": [1.], "wavelength_high": [2.7],
                          "wavelength_step": 0.5,  "wavelength_step_type": RangeStepType.Lin,
                          "use_full_wavelength_range": True, "wavelength_full_range_low": 12.,
                          "wavelength_full_range_high": 434., "background_TOF_general_start": 1.4,
                          "background_TOF_general_stop": 24.5, "background_TOF_monitor_start": {"1": 123, "2": 123},
                          "background_TOF_monitor_stop": {"1": 234, "2": 2323}, "background_TOF_roi_start": 12.,
                          "background_TOF_roi_stop": 123.}

        for key, value in list(trans_settings.items()):
            if key in trans_entries:
                value = trans_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)

        fit_settings = {"fit_type": FitType.Polynomial, "polynomial_order": 1, "wavelength_low": 12.,
                        "wavelength_high": 232.}
        if fit_entries is None:
            fit_entries = {}
        StateCalculateTransmissionTest._set_fit(state, fit_settings, fit_entries,
                                                DataType.to_string(DataType.Sample))
        StateCalculateTransmissionTest._set_fit(state, fit_settings, fit_entries,
                                                DataType.to_string(DataType.Can))
        return state

    @staticmethod
    def _get_dict(entry_name, value):
        output = {}
        if value is not None:
            output.update({entry_name: value})
        return output

    def check_bad_and_good_values(self, bad_trans=None, bad_fit=None, good_trans=None, good_fit=None):
        # Bad values
        state = self._get_calculate_transmission_state(bad_trans, bad_fit)
        assert_validate_error(self, ValueError, state)

        # Good values
        state = self._get_calculate_transmission_state(good_trans, good_fit)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_data_object(self):
        state = StateCalculateTransmissionLOQ()
        self.assertTrue(isinstance(state, StateCalculateTransmission))

    def test_that_raises_when_no_incident_monitor_is_available(self):
        self.check_bad_and_good_values(bad_trans={"incident_monitor": None, "default_incident_monitor": None},
                                       good_trans={"incident_monitor": 1, "default_incident_monitor": None})
        self.check_bad_and_good_values(bad_trans={"incident_monitor": None, "default_incident_monitor": None},
                                       good_trans={"incident_monitor": None, "default_incident_monitor": 1})

    def test_that_raises_when_no_transmission_is_specified(self):
        self.check_bad_and_good_values(bad_trans={"transmission_monitor": None, "default_transmission_monitor": None,
                                                  "transmission_radius_on_detector": None,
                                                  "transmission_roi_files": None},
                                       good_trans={"transmission_monitor": 4, "default_transmission_monitor": None,
                                                   "transmission_radius_on_detector": None,
                                                   "transmission_roi_files": None})

    def test_that_raises_for_inconsistent_prompt_peak(self):
        self.check_bad_and_good_values(bad_trans={"prompt_peak_correction_min": 1., "prompt_peak_correction_max": None},
                                       good_trans={"prompt_peak_correction_min": None,
                                                   "prompt_peak_correction_max": None})
        self.check_bad_and_good_values(bad_trans={"prompt_peak_correction_min": 1.,
                                                  "prompt_peak_correction_max": None},
                                       good_trans={"prompt_peak_correction_min": 1., "prompt_peak_correction_max": 2.})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_prompt_peak(self):
        self.check_bad_and_good_values(bad_trans={"prompt_peak_correction_min": 2., "prompt_peak_correction_max": 1.},
                                       good_trans={"prompt_peak_correction_min": 1., "prompt_peak_correction_max": 2.})

    def test_that_raises_when_not_all_elements_are_set_for_wavelength(self):
        self.check_bad_and_good_values(bad_trans={"wavelength_low": [1.], "wavelength_high": [2.],
                                                  "wavelength_step": 0.5, "wavelength_step_type": None},
                                       good_trans={"wavelength_low": [1.], "wavelength_high": [2.],
                                                   "wavelength_step": 0.5, "wavelength_step_type": RangeStepType.Lin})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_wavelength(self):
        self.check_bad_and_good_values(bad_trans={"wavelength_low": [2.], "wavelength_high": [1.],
                                                  "wavelength_step": 0.5, "wavelength_step_type":  RangeStepType.Lin},
                                       good_trans={"wavelength_low": [1.], "wavelength_high": [2.],
                                                   "wavelength_step": 0.5, "wavelength_step_type": RangeStepType.Lin})

    def test_that_raises_for_missing_full_wavelength_entry(self):
        self.check_bad_and_good_values(bad_trans={"use_full_wavelength_range": True, "wavelength_full_range_low": None,
                                                  "wavelength_full_range_high": 12.},
                                       good_trans={"use_full_wavelength_range": True, "wavelength_full_range_low": 11.,
                                                   "wavelength_full_range_high": 12.})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_full_wavelength(self):
        self.check_bad_and_good_values(bad_trans={"use_full_wavelength_range": True, "wavelength_full_range_low": 2.,
                                                  "wavelength_full_range_high": 1.},
                                       good_trans={"use_full_wavelength_range": True, "wavelength_full_range_low": 1.,
                                                   "wavelength_full_range_high": 2.})

    def test_that_raises_for_inconsistent_general_background(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_general_start": 1.,
                                                  "background_TOF_general_stop": None},
                                       good_trans={"background_TOF_general_start": None,
                                                   "background_TOF_general_stop": None})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_general_background(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_general_start": 2.,
                                                  "background_TOF_general_stop": 1.},
                                       good_trans={"background_TOF_general_start": 1.,
                                                   "background_TOF_general_stop": 2.})

    def test_that_raises_for_inconsistent_roi_background(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_roi_start": 1.,
                                                  "background_TOF_roi_stop": None},
                                       good_trans={"background_TOF_roi_start": None,
                                                   "background_TOF_roi_stop": None})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_roi_background(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_roi_start": 2.,
                                                  "background_TOF_roi_stop": 1.},
                                       good_trans={"background_TOF_roi_start": 1.,
                                                   "background_TOF_roi_stop": 2.})

    def test_that_raises_for_inconsistent_monitor_background(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_monitor_start": {"1": 12., "2": 1.},
                                                  "background_TOF_monitor_stop": None},
                                       good_trans={"background_TOF_monitor_start": None,
                                                   "background_TOF_monitor_stop": None})

    def test_that_raises_when_lengths_of_monitor_backgrounds_are_different(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_monitor_start": {"1": 1., "2": 1.},
                                                  "background_TOF_monitor_stop": {"1": 2.}},
                                       good_trans={"background_TOF_monitor_start": {"1": 1., "2": 1.},
                                                   "background_TOF_monitor_stop": {"1": 2., "2": 2.}})

    def test_that_raises_when_monitor_name_mismatch_exists_for_monitor_backgrounds(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_monitor_start": {"1": 1., "2": 1.},
                                                  "background_TOF_monitor_stop": {"1": 2., "3": 2.}},
                                       good_trans={"background_TOF_monitor_start": {"1": 1., "2": 1.},
                                                   "background_TOF_monitor_stop": {"1": 2., "2": 2.}})

    def test_that_raises_lower_bound_larger_than_upper_bound_for_monitor_backgrounds(self):
        self.check_bad_and_good_values(bad_trans={"background_TOF_monitor_start": {"1": 1., "2": 2.},
                                                  "background_TOF_monitor_stop": {"1": 2., "2": 1.}},
                                       good_trans={"background_TOF_monitor_start": {"1": 1., "2": 1.},
                                                   "background_TOF_monitor_stop": {"1": 2., "2": 2.}})

    def test_that_polynomial_order_can_only_be_set_with_polynomial_setting(self):
        self.check_bad_and_good_values(bad_fit={"fit_type": FitType.Polynomial, "polynomial_order": 0},
                                       good_fit={"fit_type": FitType.Polynomial,  "polynomial_order": 4})

    def test_that_raises_for_inconsistent_wavelength_in_fit(self):
        self.check_bad_and_good_values(bad_trans={"wavelength_low": None,  "wavelength_high": [2.]},
                                       good_trans={"wavelength_low": [1.],  "wavelength_high": [2.]})

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_wavelength_in_fit(self):
        self.check_bad_and_good_values(bad_trans={"wavelength_low": [2.],  "wavelength_high": [1.]},
                                       good_trans={"wavelength_low": [1.],  "wavelength_high": [2.]})


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateCalculateTransmissionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_calculate_transmission_builder(data_info)
        self.assertTrue(builder)

        builder.set_prompt_peak_correction_min(12.0)
        builder.set_prompt_peak_correction_max(17.0)

        builder.set_incident_monitor(1)
        builder.set_default_incident_monitor(2)
        builder.set_transmission_monitor(3)
        builder.set_default_transmission_monitor(4)
        builder.set_transmission_radius_on_detector(1.)
        builder.set_transmission_roi_files(["sdfs", "sddfsdf"])
        builder.set_transmission_mask_files(["sdfs", "bbbbbb"])

        builder.set_rebin_type(RebinType.Rebin)
        builder.set_wavelength_low([1.5])
        builder.set_wavelength_high([2.7])
        builder.set_wavelength_step(0.5)
        builder.set_wavelength_step_type(RangeStepType.Lin)
        builder.set_use_full_wavelength_range(True)
        builder.set_wavelength_full_range_low(12.)
        builder.set_wavelength_full_range_high(24.)

        builder.set_background_TOF_general_start(1.4)
        builder.set_background_TOF_general_stop(34.4)
        builder.set_background_TOF_monitor_start({"1": 123, "2": 123})
        builder.set_background_TOF_monitor_stop({"1": 234, "2": 2323})
        builder.set_background_TOF_roi_start(1.4)
        builder.set_background_TOF_roi_stop(34.4)

        builder.set_Sample_fit_type(FitType.Linear)
        builder.set_Sample_polynomial_order(0)
        builder.set_Sample_wavelength_low(10.0)
        builder.set_Sample_wavelength_high(20.0)

        builder.set_Can_fit_type(FitType.Polynomial)
        builder.set_Can_polynomial_order(3)
        builder.set_Can_wavelength_low(10.0)
        builder.set_Can_wavelength_high(20.0)

        state = builder.build()

        # Assert
        self.assertEqual(state.prompt_peak_correction_min,  12.0)
        self.assertEqual(state.prompt_peak_correction_max,  17.0)

        self.assertEqual(state.incident_monitor,  1)
        self.assertEqual(state.default_incident_monitor,  2)
        self.assertEqual(state.transmission_monitor,  3)
        self.assertEqual(state.default_transmission_monitor,  4)
        self.assertEqual(state.transmission_radius_on_detector,  1.)
        self.assertEqual(state.transmission_roi_files,  ["sdfs", "sddfsdf"])
        self.assertEqual(state.transmission_mask_files,  ["sdfs", "bbbbbb"])

        self.assertEqual(state.rebin_type, RebinType.Rebin)
        self.assertEqual(state.wavelength_low,  [1.5])
        self.assertEqual(state.wavelength_high,  [2.7])
        self.assertEqual(state.wavelength_step,  0.5)
        self.assertEqual(state.wavelength_step_type, RangeStepType.Lin)
        self.assertEqual(state.use_full_wavelength_range, True)
        self.assertEqual(state.wavelength_full_range_low,  12.)
        self.assertEqual(state.wavelength_full_range_high,  24.)

        self.assertEqual(state.background_TOF_general_start,  1.4)
        self.assertEqual(state.background_TOF_general_stop,  34.4)
        self.assertEqual(len(set(state.background_TOF_monitor_start.items()) & set({"1": 123, "2": 123}.items())), 2)
        self.assertEqual(len(set(state.background_TOF_monitor_stop.items()) & set({"1": 234, "2": 2323}.items())), 2)
        self.assertEqual(state.background_TOF_roi_start,  1.4)
        self.assertEqual(state.background_TOF_roi_stop,  34.4)

        self.assertEqual(state.fit[DataType.to_string(DataType.Sample)].fit_type, FitType.Linear)
        self.assertEqual(state.fit[DataType.to_string(DataType.Sample)].polynomial_order,  0)
        self.assertEqual(state.fit[DataType.to_string(DataType.Sample)].wavelength_low,  10.)
        self.assertEqual(state.fit[DataType.to_string(DataType.Sample)].wavelength_high,  20.)

        self.assertEqual(state.fit[DataType.to_string(DataType.Can)].fit_type, FitType.Polynomial)
        self.assertEqual(state.fit[DataType.to_string(DataType.Can)].polynomial_order,  3)
        self.assertEqual(state.fit[DataType.to_string(DataType.Can)].wavelength_low,  10.)
        self.assertEqual(state.fit[DataType.to_string(DataType.Can)].wavelength_high,  20.)

if __name__ == '__main__':
    unittest.main()
