# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import RebinType, RangeStepType, FitType, DataType, SANSFacility, SANSInstrument
from sans.state.StateObjects.StateCalculateTransmission import (
    StateCalculateTransmission,
    StateCalculateTransmissionLOQ,
    get_calculate_transmission,
)
from sans.state.StateObjects.StateData import get_data_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateCalculateTransmissionTest(unittest.TestCase):
    @staticmethod
    def _set_fit(state, default_settings, custom_settings, fit_key):
        for key, value in list(default_settings.items()):
            if key in custom_settings:
                value = custom_settings[key]

            if value is not None:  # If the value is None, then don't set it
                setattr(state.fit[fit_key.value], key, value)

    @staticmethod
    def _get_calculate_transmission_state(trans_entries, fit_entries):
        state = StateCalculateTransmission()
        if trans_entries is None:
            trans_entries = {}
        trans_settings = {
            "transmission_radius_on_detector": 12.0,
            "transmission_roi_files": ["test.xml"],
            "transmission_mask_files": ["test.xml"],
            "default_transmission_monitor": 3,
            "transmission_monitor": 4,
            "default_incident_monitor": 1,
            "incident_monitor": 2,
            "prompt_peak_correction_min": 123.0,
            "prompt_peak_correction_max": 1234.0,
            "rebin_type": RebinType.REBIN,
            "wavelength_low": [1.0],
            "wavelength_high": [2.7],
            "wavelength_step": 0.5,
            "wavelength_step_type": RangeStepType.LIN,
            "use_full_wavelength_range": True,
            "wavelength_full_range_low": 12.0,
            "wavelength_full_range_high": 434.0,
            "background_TOF_general_start": 1.4,
            "background_TOF_general_stop": 24.5,
            "background_TOF_monitor_start": {"1": 123, "2": 123},
            "background_TOF_monitor_stop": {"1": 234, "2": 2323},
            "background_TOF_roi_start": 12.0,
            "background_TOF_roi_stop": 123.0,
        }

        for key, value in list(trans_settings.items()):
            if key in trans_entries:
                value = trans_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)

        fit_settings = {"fit_type": FitType.POLYNOMIAL, "polynomial_order": 1, "wavelength_low": 12.0, "wavelength_high": 232.0}
        if fit_entries is None:
            fit_entries = {}
        StateCalculateTransmissionTest._set_fit(state, fit_settings, fit_entries, DataType.SAMPLE)
        StateCalculateTransmissionTest._set_fit(state, fit_settings, fit_entries, DataType.CAN)
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
        with self.assertRaises(ValueError):
            state.validate()

        # Good values
        state = self._get_calculate_transmission_state(good_trans, good_fit)
        self.assertIsNone(state.validate())

    def test_that_is_sans_state_data_object(self):
        state = StateCalculateTransmissionLOQ()
        self.assertTrue(isinstance(state, StateCalculateTransmission))

    def test_that_raises_when_no_incident_monitor_is_available(self):
        self.check_bad_and_good_values(
            bad_trans={"incident_monitor": None, "default_incident_monitor": None},
            good_trans={"incident_monitor": 1, "default_incident_monitor": None},
        )
        self.check_bad_and_good_values(
            bad_trans={"incident_monitor": None, "default_incident_monitor": None},
            good_trans={"incident_monitor": 1, "default_incident_monitor": 1},
        )

    def test_that_raises_when_no_transmission_is_specified(self):
        self.check_bad_and_good_values(
            bad_trans={
                "transmission_monitor": None,
                "default_transmission_monitor": None,
                "transmission_radius_on_detector": None,
                "transmission_roi_files": None,
            },
            good_trans={
                "transmission_monitor": 4,
                "default_transmission_monitor": None,
                "transmission_radius_on_detector": None,
                "transmission_roi_files": None,
            },
        )

    def test_that_raises_for_inconsistent_prompt_peak(self):
        self.check_bad_and_good_values(
            bad_trans={"prompt_peak_correction_min": 1.0, "prompt_peak_correction_max": None},
            good_trans={"prompt_peak_correction_min": None, "prompt_peak_correction_max": None},
        )
        self.check_bad_and_good_values(
            bad_trans={"prompt_peak_correction_min": 1.0, "prompt_peak_correction_max": None},
            good_trans={"prompt_peak_correction_min": 1.0, "prompt_peak_correction_max": 2.0},
        )

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_prompt_peak(self):
        self.check_bad_and_good_values(
            bad_trans={"prompt_peak_correction_min": 2.0, "prompt_peak_correction_max": 1.0},
            good_trans={"prompt_peak_correction_min": 1.0, "prompt_peak_correction_max": 2.0},
        )

    def test_that_raises_when_not_all_elements_are_set_for_wavelength(self):
        self.check_bad_and_good_values(
            bad_trans={"wavelength_low": [1.0], "wavelength_high": [2.0], "wavelength_step": 0.5, "wavelength_step_type": None},
            good_trans={
                "wavelength_low": [1.0],
                "wavelength_high": [2.0],
                "wavelength_step": 0.5,
                "wavelength_step_type": RangeStepType.LIN,
            },
        )

    def test_that_raises_for_inconsistent_general_background(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_general_start": 1.0, "background_TOF_general_stop": None},
            good_trans={"background_TOF_general_start": None, "background_TOF_general_stop": None},
        )

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_general_background(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_general_start": 2.0, "background_TOF_general_stop": 1.0},
            good_trans={"background_TOF_general_start": 1.0, "background_TOF_general_stop": 2.0},
        )

    def test_that_raises_for_inconsistent_roi_background(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_roi_start": 1.0, "background_TOF_roi_stop": None},
            good_trans={"background_TOF_roi_start": None, "background_TOF_roi_stop": None},
        )

    def test_that_raises_for_lower_bound_larger_than_upper_bound_for_roi_background(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_roi_start": 2.0, "background_TOF_roi_stop": 1.0},
            good_trans={"background_TOF_roi_start": 1.0, "background_TOF_roi_stop": 2.0},
        )

    def test_that_raises_for_inconsistent_monitor_background(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_monitor_start": {"1": 12.0, "2": 1.0}, "background_TOF_monitor_stop": None},
            good_trans={"background_TOF_monitor_start": None, "background_TOF_monitor_stop": None},
        )

    def test_that_raises_when_lengths_of_monitor_backgrounds_are_different(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 1.0}, "background_TOF_monitor_stop": {"1": 2.0}},
            good_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 1.0}, "background_TOF_monitor_stop": {"1": 2.0, "2": 2.0}},
        )

    def test_that_raises_when_monitor_name_mismatch_exists_for_monitor_backgrounds(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 1.0}, "background_TOF_monitor_stop": {"1": 2.0, "3": 2.0}},
            good_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 1.0}, "background_TOF_monitor_stop": {"1": 2.0, "2": 2.0}},
        )

    def test_that_raises_lower_bound_larger_than_upper_bound_for_monitor_backgrounds(self):
        self.check_bad_and_good_values(
            bad_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 2.0}, "background_TOF_monitor_stop": {"1": 2.0, "2": 1.0}},
            good_trans={"background_TOF_monitor_start": {"1": 1.0, "2": 1.0}, "background_TOF_monitor_stop": {"1": 2.0, "2": 2.0}},
        )

    def test_that_polynomial_order_can_only_be_set_with_polynomial_setting(self):
        self.check_bad_and_good_values(
            bad_fit={"fit_type": FitType.POLYNOMIAL, "polynomial_order": 0},
            good_fit={"fit_type": FitType.POLYNOMIAL, "polynomial_order": 4},
        )

    def test_convert_step_type_from_RANGE_LIN_to_LIN(self):
        state = StateCalculateTransmission()
        state.wavelength_step_type = RangeStepType.RANGE_LIN
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LIN)

    def test_convert_step_type_from_RANGE_LOG_to_LOG(self):
        state = StateCalculateTransmission()
        state.wavelength_step_type = RangeStepType.RANGE_LOG
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LOG)

    def test_convert_step_type_does_not_change_LIN(self):
        state = StateCalculateTransmission()
        state.wavelength_step_type = RangeStepType.LIN
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LIN)

    def test_convert_step_type_does_not_change_LOG(self):
        state = StateCalculateTransmission()
        state.wavelength_step_type = RangeStepType.LOG
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.LOG)

    def test_convert_step_type_does_not_change_NOT_SET(self):
        state = StateCalculateTransmission()
        state.wavelength_step_type = RangeStepType.NOT_SET
        self.assertEqual(state.wavelength_step_type_lin_log, RangeStepType.NOT_SET)


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
        state_transmission = get_calculate_transmission(data_info.instrument)

        state_transmission.prompt_peak_correction_min = 12.0
        state_transmission.prompt_peak_correction_max = 17.0

        state_transmission.incident_monitor = 1
        state_transmission.default_incident_monitor = 2
        state_transmission.transmission_monitor = 3
        state_transmission.default_transmission_monitor = 4
        state_transmission.transmission_radius_on_detector = 1.0
        state_transmission.transmission_roi_files = ["sdfs", "sddfsdf"]
        state_transmission.transmission_mask_files = ["sdfs", "bbbbbb"]

        state_transmission.rebin_type = RebinType.REBIN
        state_transmission.wavelength_low = [1.5]
        state_transmission.wavelength_high = [2.7]
        state_transmission.wavelength_interval.wavelength_step = 0.5
        state_transmission.wavelength_step_type = RangeStepType.LIN
        state_transmission.use_full_wavelength_range = True
        state_transmission.wavelength_full_range_low = 12.0
        state_transmission.wavelength_full_range_high = 24.0

        state_transmission.background_TOF_general_start = 1.4
        state_transmission.background_TOF_general_stop = 34.4
        state_transmission.background_TOF_monitor_start = {"1": 123, "2": 123}
        state_transmission.background_TOF_monitor_stop = {"1": 234, "2": 2323}
        state_transmission.background_TOF_roi_start = 1.4
        state_transmission.background_TOF_roi_stop = 34.4

        state_transmission.set_sample_fit_type(FitType.LINEAR)
        state_transmission.set_sample_polynomial_order(0)
        state_transmission.set_sample_wavelength_low(10.0)
        state_transmission.set_sample_wavelength_high(20.0)

        state_transmission.set_can_fit_type(FitType.POLYNOMIAL)
        state_transmission.set_can_polynomial_order(3)
        state_transmission.set_can_wavelength_low(10.0)
        state_transmission.set_can_wavelength_high(20.0)

        # Assert
        self.assertEqual(state_transmission.prompt_peak_correction_min, 12.0)
        self.assertEqual(state_transmission.prompt_peak_correction_max, 17.0)

        self.assertEqual(state_transmission.incident_monitor, 1)
        self.assertEqual(state_transmission.default_incident_monitor, 2)
        self.assertEqual(state_transmission.transmission_monitor, 3)
        self.assertEqual(state_transmission.default_transmission_monitor, 4)
        self.assertEqual(state_transmission.transmission_radius_on_detector, 1.0)
        self.assertEqual(state_transmission.transmission_roi_files, ["sdfs", "sddfsdf"])
        self.assertEqual(state_transmission.transmission_mask_files, ["sdfs", "bbbbbb"])

        self.assertEqual(state_transmission.rebin_type, RebinType.REBIN)
        self.assertEqual(state_transmission.wavelength_low, [1.5])
        self.assertEqual(state_transmission.wavelength_high, [2.7])
        self.assertEqual(state_transmission.wavelength_interval.wavelength_step, 0.5)
        self.assertEqual(state_transmission.wavelength_step_type, RangeStepType.LIN)
        self.assertEqual(state_transmission.use_full_wavelength_range, True)
        self.assertEqual(state_transmission.wavelength_full_range_low, 12.0)
        self.assertEqual(state_transmission.wavelength_full_range_high, 24.0)

        self.assertEqual(state_transmission.background_TOF_general_start, 1.4)
        self.assertEqual(state_transmission.background_TOF_general_stop, 34.4)
        self.assertEqual(len(set(state_transmission.background_TOF_monitor_start.items()) & set({"1": 123, "2": 123}.items())), 2)
        self.assertEqual(len(set(state_transmission.background_TOF_monitor_stop.items()) & set({"1": 234, "2": 2323}.items())), 2)
        self.assertEqual(state_transmission.background_TOF_roi_start, 1.4)
        self.assertEqual(state_transmission.background_TOF_roi_stop, 34.4)

        self.assertEqual(state_transmission.fit[DataType.SAMPLE.value].fit_type, FitType.LINEAR)
        self.assertEqual(state_transmission.fit[DataType.SAMPLE.value].polynomial_order, 0)
        self.assertEqual(state_transmission.fit[DataType.SAMPLE.value].wavelength_low, 10.0)
        self.assertEqual(state_transmission.fit[DataType.SAMPLE.value].wavelength_high, 20.0)

        self.assertEqual(state_transmission.fit[DataType.CAN.value].fit_type, FitType.POLYNOMIAL)
        self.assertEqual(state_transmission.fit[DataType.CAN.value].polynomial_order, 3)
        self.assertEqual(state_transmission.fit[DataType.CAN.value].wavelength_low, 10.0)
        self.assertEqual(state_transmission.fit[DataType.CAN.value].wavelength_high, 20.0)


if __name__ == "__main__":
    unittest.main()
