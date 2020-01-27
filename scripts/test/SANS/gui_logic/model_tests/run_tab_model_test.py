# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from sans.common.enums import (ReductionDimensionality, ReductionMode, RangeStepType, SampleShape, SaveType,
                               SANSInstrument)
from sans.gui_logic.models.RunTabModel import RunTabModel
from sans.user_file.settings_tags import (OtherId, event_binning_string_values, DetectorId)
from sans.user_file.settings_tags import (det_fit_range)


class RunTabModelTest(unittest.TestCase):
    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================
    def test_that_default_instrument_is_NoInstrument(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.instrument, SANSInstrument.NO_INSTRUMENT)

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_compatibility_mode_is_true(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(run_tab_model.compatibility_mode)

    def test_that_can_set_compatibility_mode(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.compatibility_mode = False
        self.assertFalse(run_tab_model.compatibility_mode)

    def test_that_default_event_slice_optimisation_is_false(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertFalse(run_tab_model.event_slice_optimisation)

    def test_that_can_set_event_slice_optimisation(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.event_slice_optimisation = True
        self.assertTrue(run_tab_model.event_slice_optimisation)

    # ------------------------------------------------------------------------------------------------------------------
    # Save options
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_can_zero_error_free_saving_is_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(run_tab_model.zero_error_free)

    def test_that_can_zero_error_free_saving_can_be_changed(self):
        run_tab_model = RunTabModel({OtherId.SAVE_AS_ZERO_ERROR_FREE: [True]})
        run_tab_model.zero_error_free = False
        self.assertFalse(run_tab_model.zero_error_free)

    def test_that_default_save_type_is_NXcanSAS(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.save_types, [SaveType.NX_CAN_SAS])

    def test_that_can_select_multiple_save_types(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.save_types = [SaveType.RKH, SaveType.NX_CAN_SAS]
        self.assertEqual(run_tab_model.save_types, [SaveType.RKH, SaveType.NX_CAN_SAS])

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Event slices
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_if_no_slice_event_is_present_an_empty_string_is_returned(self):
        run_tab_model = RunTabModel({"test": 1})
        self.assertEqual(run_tab_model.event_slices, "")

    def test_that_slice_event_can_be_retrieved_if_it_exists(self):
        run_tab_model = RunTabModel({OtherId.EVENT_SLICES: [event_binning_string_values(value="test")]})
        self.assertEqual(run_tab_model.event_slices, "test")

    def test_that_slice_event_can_be_updated(self):
        run_tab_model = RunTabModel({OtherId.EVENT_SLICES: [event_binning_string_values(value="test")]})
        run_tab_model.event_slices = "test2"
        self.assertEqual(run_tab_model.event_slices, "test2")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_1D_reduction_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.reduction_dimensionality, ReductionDimensionality.ONE_DIM)

    def test_that_is_set_to_2D_reduction(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.reduction_dimensionality = ReductionDimensionality.TWO_DIM
        self.assertEqual(run_tab_model.reduction_dimensionality, ReductionDimensionality.TWO_DIM)

    def test_that_raises_when_not_setting_with_reduction_dim_enum(self):
        def red_dim_wrapper():
            run_tab_model = RunTabModel({"test": [1]})
            run_tab_model.reduction_dimensionality = "string"
        self.assertRaises(ValueError, red_dim_wrapper)

    def test_that_can_update_reduction_dimensionality(self):
        run_tab_model = RunTabModel({OtherId.REDUCTION_DIMENSIONALITY: [ReductionDimensionality.ONE_DIM]})
        self.assertEqual(run_tab_model.reduction_dimensionality, ReductionDimensionality.ONE_DIM)
        run_tab_model.reduction_dimensionality = ReductionDimensionality.TWO_DIM
        self.assertEqual(run_tab_model.reduction_dimensionality, ReductionDimensionality.TWO_DIM)

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_event_binning_default_settings_are_emtpy(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.event_binning, "")

    def test_that_event_binning_can_be_set(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.event_binning = "1,-1,10"
        self.assertEqual(run_tab_model.event_binning, "1,-1,10")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_set_to_lab_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.reduction_mode, ReductionMode.LAB)

    def test_that_can_be_set_to_something_else(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.reduction_mode = ReductionMode.MERGED
        self.assertEqual(run_tab_model.reduction_mode, ReductionMode.MERGED)

    def test_that_raises_when_setting_with_wrong_input(self):
        def red_mode_wrapper():
            run_tab_model = RunTabModel({"test": [1]})
            run_tab_model.reduction_mode = "string"
        self.assertRaises(ValueError, red_mode_wrapper)

    def test_that_can_update_reduction_mode(self):
        run_tab_model = RunTabModel({DetectorId.REDUCTION_MODE: [ReductionMode.HAB]})
        self.assertEqual(run_tab_model.reduction_mode, ReductionMode.HAB)
        run_tab_model.reduction_mode = ReductionMode.ALL
        self.assertEqual(run_tab_model.reduction_mode, ReductionMode.ALL)

    # ------------------------------------------------------------------------------------------------------------------
    # Merge range
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_merge_mask_is_false_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertFalse(run_tab_model.merge_mask)

    def test_that_merge_mask_can_be_set_to_something_else(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.merge_mask = True
        self.assertTrue(run_tab_model.merge_mask)

    def test_that_merge_min_is_None_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.merge_min, None)

    def test_that_merge_max_is_None_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.merge_max, None)

    def test_that_merge_min_can_be_set(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.merge_min = 78.9
        self.assertEqual(run_tab_model.merge_min, 78.9)

    def test_that_merge_range_set_correctly(self):
        run_tab_model = RunTabModel({DetectorId.MERGE_RANGE: [det_fit_range(use_fit=True, start=0.13, stop=0.15)]})
        self.assertEqual(run_tab_model.merge_min, 0.13)
        self.assertEqual(run_tab_model.merge_max, 0.15)
        self.assertTrue(run_tab_model.merge_mask)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_defaults_for_merge_are_empty_and_false(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.merge_scale, "1.0")
        self.assertEqual(run_tab_model.merge_shift, "0.0")
        self.assertFalse(run_tab_model.merge_scale_fit)
        self.assertFalse(run_tab_model.merge_shift_fit)
        self.assertEqual(run_tab_model.merge_q_range_start, "")
        self.assertEqual(run_tab_model.merge_q_range_stop, "")

    def test_that_can_set_and_reset_merged_settings(self):
        run_tab_model = RunTabModel({DetectorId.SHIFT_FIT: [det_fit_range(start=1., stop=2., use_fit=True)],
                                     DetectorId.RESCALE_FIT: [det_fit_range(start=1.4, stop=7., use_fit=False)],
                                     DetectorId.RESCALE: [12.],
                                     DetectorId.SHIFT: [234.]})
        self.assertEqual(run_tab_model.merge_scale, 12.)
        self.assertEqual(run_tab_model.merge_shift, 234.)
        self.assertFalse(run_tab_model.merge_scale_fit)
        self.assertTrue(run_tab_model.merge_shift_fit)
        self.assertEqual(run_tab_model.merge_q_range_start, 1.)
        self.assertEqual(run_tab_model.merge_q_range_stop, 7.)

        run_tab_model.merge_scale = 12.3
        run_tab_model.merge_shift = 3.
        run_tab_model.merge_scale_fit = True
        run_tab_model.merge_shift_fit = False
        run_tab_model.merge_q_range_start = 2.
        run_tab_model.merge_q_range_stop = 8.

        self.assertEqual(run_tab_model.merge_scale, 12.3)
        self.assertEqual(run_tab_model.merge_shift, 3.)
        self.assertTrue(run_tab_model.merge_scale_fit)
        self.assertFalse(run_tab_model.merge_shift_fit)
        self.assertEqual(run_tab_model.merge_q_range_start, 2.)
        self.assertEqual(run_tab_model.merge_q_range_stop, 8.)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_wavelength_settings_are_empty(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(not run_tab_model.wavelength_min)
        self.assertTrue(not run_tab_model.wavelength_max)
        self.assertTrue(not run_tab_model.wavelength_step)

    def test_that_default_wavelength_step_type_is_linear(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.wavelength_step_type, RangeStepType.LIN)

    def test_that_can_set_wavelength(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.wavelength_min = 1.
        run_tab_model.wavelength_max = 2.
        run_tab_model.wavelength_step = .5
        run_tab_model.wavelength_step_type = RangeStepType.LIN
        run_tab_model.wavelength_step_type = RangeStepType.LOG
        self.assertEqual(run_tab_model.wavelength_min, 1.)
        self.assertEqual(run_tab_model.wavelength_max, 2.)
        self.assertEqual(run_tab_model.wavelength_step, .5)
        self.assertEqual(run_tab_model.wavelength_step_type, RangeStepType.LOG)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_absolute_scale_has_an_empty_default_value(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(not run_tab_model.absolute_scale)

    def test_that_can_set_absolute_scale(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.absolute_scale = .5
        self.assertEqual(run_tab_model.absolute_scale, .5)

    def test_that_default_extents_are_empty(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(not run_tab_model.sample_width)
        self.assertTrue(not run_tab_model.sample_height)
        self.assertTrue(not run_tab_model.sample_thickness)
        self.assertTrue(not run_tab_model.z_offset)

    def test_that_default_sample_shape_is_cylinder_axis_up(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.sample_shape, None)

    def test_that_can_set_the_sample_geometry(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.sample_width = 1.2
        run_tab_model.sample_height = 1.6
        run_tab_model.sample_thickness = 1.8
        run_tab_model.z_offset = 1.78
        run_tab_model.sample_shape = SampleShape.FLAT_PLATE
        self.assertEqual(run_tab_model.sample_width, 1.2)
        self.assertEqual(run_tab_model.sample_height, 1.6)
        self.assertEqual(run_tab_model.sample_thickness, 1.8)
        self.assertEqual(run_tab_model.z_offset, 1.78)
        self.assertEqual(run_tab_model.sample_shape, SampleShape.FLAT_PLATE)

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q limits
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_limits_default_to_empty(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.q_1d_rebin_string, "")

        self.assertEqual(run_tab_model.q_xy_max, "")
        self.assertEqual(run_tab_model.q_xy_step, "")
        self.assertEqual(run_tab_model.q_xy_step_type, None)
        self.assertEqual(run_tab_model.r_cut, "")
        self.assertEqual(run_tab_model.w_cut, "")

    def test_that_can_set_the_q_limits(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.q_1d_rebin_string = "test"
        run_tab_model.q_xy_max = 1.
        run_tab_model.q_xy_step = 122.
        run_tab_model.q_xy_step_type = RangeStepType.LOG
        run_tab_model.r_cut = 45.
        run_tab_model.w_cut = 890.

        self.assertEqual(run_tab_model.q_1d_rebin_string, "test")
        self.assertEqual(run_tab_model.q_xy_max, 1.)
        self.assertEqual(run_tab_model.q_xy_step, 122.)
        self.assertEqual(run_tab_model.q_xy_step_type, RangeStepType.LOG)
        self.assertEqual(run_tab_model.r_cut, 45.)
        self.assertEqual(run_tab_model.w_cut, 890.)

    def test_that_q_1d_rebin_string_as_bytes_is_converted_to_string(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.q_1d_rebin_string = b"test"

        q_1d_rebin_string = run_tab_model.q_1d_rebin_string
        self.assertEqual(type(q_1d_rebin_string), str)
        self.assertEqual(q_1d_rebin_string, "test")

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_gravity_extra_length_empty_by_default_and_usage_true_by_default(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertTrue(run_tab_model.gravity_on_off)
        self.assertEqual(run_tab_model.gravity_extra_length, "")

    def test_that_can_set_gravity(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.gravity_on_off = False
        run_tab_model.gravity_extra_length = 1.
        self.assertFalse(run_tab_model.gravity_on_off)
        self.assertEqual(run_tab_model.gravity_extra_length, 1.)

    # ------------------------------------------------------------------------------------------------------------------
    # Q resolution
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_resolution_settings_show_empty_defaults(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertFalse(run_tab_model.use_q_resolution)
        self.assertEqual(run_tab_model.q_resolution_source_a, "")
        self.assertEqual(run_tab_model.q_resolution_sample_a, "")
        self.assertEqual(run_tab_model.q_resolution_source_h, "")
        self.assertEqual(run_tab_model.q_resolution_sample_h, "")
        self.assertEqual(run_tab_model.q_resolution_source_w, "")
        self.assertEqual(run_tab_model.q_resolution_sample_w, "")
        self.assertEqual(run_tab_model.q_resolution_collimation_length, "")
        self.assertEqual(run_tab_model.q_resolution_delta_r, "")
        self.assertEqual(run_tab_model.q_resolution_moderator_file, "")

    def test_that_q_resolution_can_be_set_correctly(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.use_q_resolution = True
        run_tab_model.q_resolution_source_a = 1.5
        run_tab_model.q_resolution_sample_a = 2.5
        run_tab_model.q_resolution_source_h = 1.5
        run_tab_model.q_resolution_sample_h = 2.5
        run_tab_model.q_resolution_source_w = 1.5
        run_tab_model.q_resolution_sample_w = 2.5
        run_tab_model.q_resolution_collimation_length = 1.7
        run_tab_model.q_resolution_delta_r = 12.4
        run_tab_model.q_resolution_moderator_file = "test.txt"

        self.assertTrue(run_tab_model.use_q_resolution)
        self.assertEqual(run_tab_model.q_resolution_source_a, 1.5)
        self.assertEqual(run_tab_model.q_resolution_sample_a, 2.5)
        self.assertEqual(run_tab_model.q_resolution_source_h, 1.5)
        self.assertEqual(run_tab_model.q_resolution_sample_h, 2.5)
        self.assertEqual(run_tab_model.q_resolution_source_w, 1.5)
        self.assertEqual(run_tab_model.q_resolution_sample_w, 2.5)
        self.assertEqual(run_tab_model.q_resolution_collimation_length, 1.7)
        self.assertEqual(run_tab_model.q_resolution_delta_r, 12.4)
        self.assertEqual(run_tab_model.q_resolution_moderator_file, "test.txt")

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi mask
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_phi_mask_defaults_to_90_and_true_for_use_mirror(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.phi_limit_min, "-90")
        self.assertEqual(run_tab_model.phi_limit_max, "90")
        self.assertTrue(run_tab_model.phi_limit_use_mirror)

    def test_that_phi_mask_can_be_set(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.phi_limit_min = 12.
        run_tab_model.phi_limit_max = 13.
        run_tab_model.phi_limit_use_mirror = True
        self.assertEqual(run_tab_model.phi_limit_min, 12.)
        self.assertEqual(run_tab_model.phi_limit_max, 13.)
        self.assertTrue(run_tab_model.phi_limit_use_mirror)

    # ------------------------------------------------------------------------------------------------------------------
    # Radius mask
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_radius_mask_defaults_to_empty(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.radius_limit_min, "")
        self.assertEqual(run_tab_model.radius_limit_max, "")

    def test_that_radius_mask_can_be_set(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.radius_limit_min = 12.
        run_tab_model.radius_limit_max = 13.
        self.assertEqual(run_tab_model.radius_limit_min, 12.)
        self.assertEqual(run_tab_model.radius_limit_max, 13.)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_mask_file_defaults_are_empty(self):
        run_tab_model = RunTabModel({"test": [1]})
        self.assertEqual(run_tab_model.mask_files, [])

    def test_that_mask_file_can_be_set(self):
        run_tab_model = RunTabModel({"test": [1]})
        run_tab_model.mask_files = ["file.txt", "file2.txt"]
        self.assertEqual(run_tab_model.mask_files, ["file.txt", "file2.txt"])


if __name__ == '__main__':
    unittest.main()
