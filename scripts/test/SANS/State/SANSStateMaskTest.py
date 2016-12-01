import unittest
import mantid

from SANS2.State.SANSStateMask import (SANSStateMask, SANSStateMaskISIS)
from SANS2.Common.SANSConstants import SANSConstants
from StateTestHelper import (assert_validate_error, assert_raises_nothing)

EXPLICIT_NONE = "explict_none"


class SANSStateMaskTest(unittest.TestCase):
    @staticmethod
    def _set_detector(state, default_settings, custom_settings, detector_key):
        detector = state.detectors[detector_key]
        for key, value in default_settings.items():
            if key in custom_settings:
                value = custom_settings[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(detector, key, value)
        state.detectors[detector_key] = detector

    @staticmethod
    def _get_mask_state(general_entries, detector_entries):
        state = SANSStateMaskISIS()
        # Setup the general mask settings
        mask_settings = {"radius_min": 12., "radius_max": 17.,
                         "bin_mask_general_start": [1., 2., 3.], "bin_mask_general_stop": [2., 3., 4.],
                         "mask_files": None,
                         "phi_min": 0.5, "phi_max": 1., "use_mask_phi_mirror": True,
                         "beam_stop_arm_width": 1., "beam_stop_arm_angle": 24.5, "beam_stop_arm_pos1": 12.,
                         "beam_stop_arm_pos2": 34.,
                         "clear": False, "clear_time": False, "single_spectra": [1, 4, 6],
                         "spectrum_range_start": [1, 5, 7], "spectrum_range_stop": [2, 6, 8],
                         "idf_path": ""}

        for key, value in mask_settings.items():
            if key in general_entries:
                value = general_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)

        # Now setup the detector-specific settings
        detector_settings = {"single_vertical_strip_mask": [1, 2, 4], "range_vertical_strip_start": [1, 2, 4],
                             "range_vertical_strip_stop": [2, 3, 5], "single_horizontal_strip_mask": [1, 2, 4],
                             "range_horizontal_strip_start": [1, 2, 4], "range_horizontal_strip_stop": [2, 3, 5],
                             "block_horizontal_start": [1, 2, 4], "block_horizontal_stop": [2, 3, 5],
                             "block_vertical_start": [1, 2, 4], "block_vertical_stop": [2, 3, 5],
                             "block_cross_horizontal": [1, 2, 4], "block_cross_vertical": [2, 3, 5],
                             "bin_mask_start": [1., 2., 4.], "bin_mask_stop": [2., 3., 5.],
                             "detector_name": "name", "detector_name_short": "name_short"}

        SANSStateMaskTest._set_detector(state, detector_settings, detector_entries, SANSConstants.low_angle_bank)
        SANSStateMaskTest._set_detector(state, detector_settings, detector_entries, SANSConstants.high_angle_bank)

        return state

    @staticmethod
    def _get_dict(entry_name, value):
        is_explicit_none = value == EXPLICIT_NONE
        output = {}
        if value is not None:
            value = None if is_explicit_none else value
            output.update({entry_name: value})
        return output

    def assert_raises_for_bad_value_and_raises_nothing_for_good_value(self, entry_name=None, bad_value_general=None,
                                                                      bad_value_detector=None, good_value_general=None,
                                                                      good_value_detector=None):
        # Bad values
        bad_value_general_dict = SANSStateMaskTest._get_dict(entry_name, bad_value_general)
        bad_value_detector_dict = SANSStateMaskTest._get_dict(entry_name, bad_value_detector)
        state = self._get_mask_state(bad_value_general_dict, bad_value_detector_dict)
        assert_validate_error(self, ValueError, state)

        # Good values
        good_value_general_dict = SANSStateMaskTest._get_dict(entry_name, good_value_general)
        good_value_detector_dict = SANSStateMaskTest._get_dict(entry_name, good_value_detector)
        state = self._get_mask_state(good_value_general_dict, good_value_detector_dict)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_data_object(self):
        state = SANSStateMaskISIS()
        self.assertTrue(isinstance(state, SANSStateMask))

    def test_that_raises_when_lower_radius_bound_larger_than_upper_bound(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("radius_min", 500., None, 12., None)

    def test_that_raises_when_only_one_bin_mask_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_general_start", EXPLICIT_NONE,
                                                                           None, [1., 2., 3.], None)

    def test_that_raises_when_bin_mask_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_general_start", [1., 3.],
                                                                           None, [1., 2., 3.], None)

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_bin_mask(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_general_start", [1., 10., 3.],
                                                                           None, [1., 2., 3.], None)

    def test_that_raises_when_only_one_spectrum_range_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("spectrum_range_start", EXPLICIT_NONE,
                                                                           None, [1, 5, 7], None)

    def test_that_raises_when_spectrum_range_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("spectrum_range_start", [1, 3],
                                                                           None, [1, 5, 7], None)

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_spectrum_range(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("spectrum_range_start", [1, 10, 3],
                                                                           None, [1, 5, 7], None)

    def test_that_raises_when_only_one_vertical_strip_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_vertical_strip_start", None,
                                                                           EXPLICIT_NONE, None, [1, 2, 4])

    def test_that_raises_when_vertical_strip_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_vertical_strip_start", None, [1, 2],
                                                                           None, [1, 2, 4])

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_vertical_strip(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_vertical_strip_start", None,
                                                                           [1, 10, 3], None, [1, 2, 4])

    def test_that_raises_when_only_one_horizontal_strip_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_horizontal_strip_start", None,
                                                                           EXPLICIT_NONE, None, [1, 2, 4])

    def test_that_raises_when_horizontal_strip_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_horizontal_strip_start", None, [1, 2],
                                                                           None, [1, 2, 4])

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_horizontal_strip(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("range_horizontal_strip_start", None,
                                                                           [1, 10, 3], None, [1, 2, 4])

    def test_that_raises_when_only_one_horizontal_block_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_horizontal_start", None,
                                                                           EXPLICIT_NONE, None, [1, 2, 4])

    def test_that_raises_when_horizontal_block_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_horizontal_start", None, [1, 2],
                                                                           None, [1, 2, 4])

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_horiztonal_block(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_horizontal_start", None,
                                                                           [1, 10, 3], None, [1, 2, 4])

    def test_that_raises_when_only_one_vertical_block_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_vertical_start", None,
                                                                           EXPLICIT_NONE, None, [1, 2, 4])

    def test_that_raises_when_vertical_block_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_vertical_start", None,
                                                                           [1, 2],
                                                                           None, [1, 2, 4])

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_vertical_block(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("block_vertical_start", None,
                                                                           [1, 10, 3], None, [1, 2, 4])

    def test_that_raises_when_only_one_time_mask_has_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_start", None,
                                                                           EXPLICIT_NONE, None, [1., 2., 4.])

    def test_that_raises_when_time_mask_lengths_are_mismatched(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_start", None,
                                                                           [1., 2.],
                                                                           None, [1., 2., 4.])

    def test_that_raises_lower_bound_is_larger_than_upper_bound_for_time_mask(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("bin_mask_start", None,
                                                                           [1., 10., 3.], None, [1., 2., 4.])

    def test_that_raises_if_detector_names_have_not_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("detector_name", None,
                                                                           EXPLICIT_NONE, None, "name")

    def test_that_raises_if_short_detector_names_have_not_been_set(self):
        self.assert_raises_for_bad_value_and_raises_nothing_for_good_value("detector_name_short", None,
                                                                           EXPLICIT_NONE, None, "name")


if __name__ == '__main__':
    unittest.main()
