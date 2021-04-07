# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import (ReductionDimensionality, ReductionMode, RangeStepType, SampleShape, SaveType,
                               SANSInstrument, FitModeForMerge, DetectorType)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.state.AllStates import AllStates
from sans.state.StateObjects.StateMoveDetectors import StateMoveDetectors


class StateGuiModelTest(unittest.TestCase):
    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================
    def test_that_default_instrument_is_NoInstrument(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.instrument, SANSInstrument.NO_INSTRUMENT)

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_compatibility_mode_is_true(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(state_gui_model.compatibility_mode)

    def test_that_can_set_compatibility_mode(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.compatibility_mode = False
        self.assertFalse(state_gui_model.compatibility_mode)

    def test_that_default_event_slice_optimisation_is_false(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertFalse(state_gui_model.event_slice_optimisation)

    def test_that_can_set_event_slice_optimisation(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.event_slice_optimisation = True
        self.assertTrue(state_gui_model.event_slice_optimisation)

    # ------------------------------------------------------------------------------------------------------------------
    # Save options
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_can_zero_error_free_saving_is_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(state_gui_model.zero_error_free)

    def test_that_can_zero_error_free_saving_can_be_changed(self):
        state = AllStates()
        state.save.zero_free_correction = True
        state_gui_model = StateGuiModel(state)
        self.assertTrue(state_gui_model.zero_error_free)
        state_gui_model.zero_error_free = False
        self.assertFalse(state_gui_model.all_states.save.zero_free_correction)

    def test_that_default_save_type_is_NXcanSAS(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.save_types, [SaveType.NX_CAN_SAS])

    def test_that_can_select_multiple_save_types(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.save_types = [SaveType.RKH, SaveType.NX_CAN_SAS]
        self.assertEqual(state_gui_model.save_types, [SaveType.RKH, SaveType.NX_CAN_SAS])

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Event slices
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_if_no_slice_event_is_present_an_empty_string_is_returned(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.event_slices, "")

    def test_that_slice_event_can_be_retrieved_if_it_exists(self):
        state = AllStates()
        state.slice.event_slice_str = "test"
        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.event_slices, "test")

    def test_that_slice_event_can_be_updated(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.event_slices = "1-2"
        self.assertEqual(state_gui_model.all_states.slice.event_slice_str, "1-2")
        self.assertEqual(state_gui_model.all_states.slice.start_time, [1.0])
        self.assertEqual(state_gui_model.all_states.slice.end_time, [2.0])

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_1D_reduction_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.ONE_DIM)

    def test_that_is_set_to_2D_reduction(self):
        state_gui_model = StateGuiModel(AllStates())
        expected = ReductionDimensionality.TWO_DIM
        state_gui_model.reduction_dimensionality = expected

        self.assertEqual(expected, state_gui_model.reduction_dimensionality)
        self.assertEqual(expected, state_gui_model.all_states.convert_to_q.reduction_dimensionality)
        self.assertEqual(expected, state_gui_model.all_states.reduction.reduction_dimensionality)

    def test_that_raises_when_not_setting_with_reduction_dim_enum(self):
        def red_dim_wrapper():
            state_gui_model = StateGuiModel(AllStates())
            state_gui_model.reduction_dimensionality = "string"

        self.assertRaises(ValueError, red_dim_wrapper)

    def test_that_can_update_reduction_dimensionality(self):
        state = AllStates()
        state.reduction.reduction_dimensionality = ReductionDimensionality.ONE_DIM
        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.ONE_DIM)
        state_gui_model.reduction_dimensionality = ReductionDimensionality.TWO_DIM
        self.assertEqual(state_gui_model.all_states.reduction.reduction_dimensionality,
                         ReductionDimensionality.TWO_DIM)

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_event_binning_default_settings_are_emtpy(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.event_binning, "")

    def test_that_event_binning_can_be_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.event_binning = "1,-1,10"
        self.assertEqual(state_gui_model.event_binning, "1,-1,10")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_set_to_lab_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.reduction_mode, ReductionMode.LAB)

    def test_that_can_be_set_to_something_else(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.reduction_mode = ReductionMode.MERGED
        self.assertEqual(state_gui_model.reduction_mode, ReductionMode.MERGED)

    def test_that_raises_when_setting_with_wrong_input(self):
        def red_mode_wrapper():
            state_gui_model = StateGuiModel(AllStates())
            state_gui_model.reduction_mode = "string"

        self.assertRaises(ValueError, red_mode_wrapper)

    def test_that_can_update_reduction_mode(self):
        state = AllStates()
        state.reduction.reduction_mode = ReductionMode.HAB
        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.reduction_mode, ReductionMode.HAB)
        state_gui_model.reduction_mode = ReductionMode.ALL
        self.assertEqual(state_gui_model.all_states.reduction.reduction_mode, ReductionMode.ALL)

    # ------------------------------------------------------------------------------------------------------------------
    # Merge range
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_merge_mask_is_false_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertFalse(state_gui_model.merge_mask)

    def test_that_merge_mask_can_be_set_to_something_else(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.merge_mask = True
        self.assertTrue(state_gui_model.merge_mask)

    def test_that_merge_min_is_None_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.merge_min, None)

    def test_that_merge_max_is_None_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.merge_max, None)

    def test_that_merge_min_can_be_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.merge_min = 78.9
        self.assertEqual(state_gui_model.merge_min, 78.9)

    def test_that_merge_range_set_correctly(self):
        state = AllStates()
        state.reduction.merge_min = 0.13
        state.reduction.merge_max = 0.15
        state.reduction.merge_mask = True
        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.merge_min, 0.13)
        self.assertEqual(state_gui_model.merge_max, 0.15)
        self.assertTrue(state_gui_model.merge_mask)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_defaults_for_merge_are_empty_and_false(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.merge_scale, 1.0)
        self.assertEqual(state_gui_model.merge_shift, 0.0)
        self.assertFalse(state_gui_model.merge_scale_fit)
        self.assertFalse(state_gui_model.merge_shift_fit)
        self.assertEqual(state_gui_model.merge_q_range_start, "")
        self.assertEqual(state_gui_model.merge_q_range_stop, "")

    def test_that_can_set_and_reset_merged_settings(self):
        state = AllStates()
        state.reduction.merge_scale = 12.
        state.reduction.merge_shift = 234.
        state.reduction.merge_fit_mode = FitModeForMerge.SHIFT_ONLY
        state.reduction.merge_range_min = 1.
        state.reduction.merge_range_max = 7.

        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.merge_scale, 12.)
        self.assertEqual(state_gui_model.merge_shift, 234.)
        self.assertFalse(state_gui_model.merge_scale_fit)
        self.assertTrue(state_gui_model.merge_shift_fit)
        self.assertEqual(state_gui_model.merge_q_range_start, 1.)
        self.assertEqual(state_gui_model.merge_q_range_stop, 7.)

        state_gui_model.merge_scale = 12.3
        state_gui_model.merge_shift = 3.
        state_gui_model.merge_scale_fit = True
        state_gui_model.merge_shift_fit = False
        state_gui_model.merge_q_range_start = 2.
        state_gui_model.merge_q_range_stop = 8.

        self.assertEqual(state_gui_model.merge_scale, 12.3)
        self.assertEqual(state_gui_model.merge_shift, 3.)
        self.assertTrue(state_gui_model.merge_scale_fit)
        self.assertFalse(state_gui_model.merge_shift_fit)
        self.assertEqual(state_gui_model.merge_q_range_start, 2.)
        self.assertEqual(state_gui_model.merge_q_range_stop, 8.)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_wavelength_settings_are_empty(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(not state_gui_model.wavelength_min)
        self.assertTrue(not state_gui_model.wavelength_max)
        self.assertTrue(not state_gui_model.wavelength_step)

    def test_that_wavelength_step_type_defaults_to_linear_if_none(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.wavelength_step_type, RangeStepType.LIN)

    def test_that_wavelength_step_type_defaults_to_linear_if_not_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.wavelength_step_type = RangeStepType.NOT_SET
        self.assertEqual(state_gui_model.wavelength_step_type, RangeStepType.LIN)

    def test_that_can_set_wavelength(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.wavelength_min = 1.
        state_gui_model.wavelength_max = 2.
        state_gui_model.wavelength_step = .5
        state_gui_model.wavelength_step_type = RangeStepType.LIN
        state_gui_model.wavelength_step_type = RangeStepType.LOG
        self._assert_all_wavelengths_match(state_gui_model, [1.], [2.], .5, RangeStepType.LOG)

    def _assert_all_wavelengths_match(self, model, low, high, step, step_type):
        # Transmission
        self.assertEqual(model.all_states.adjustment.calculate_transmission.wavelength_low, low)
        self.assertEqual(model.all_states.adjustment.calculate_transmission.wavelength_high, high)
        self.assertEqual(model.all_states.adjustment.calculate_transmission.wavelength_step, step)
        self.assertEqual(model.all_states.adjustment.calculate_transmission.wavelength_step_type, step_type)
        # Monitor
        self.assertEqual(model.all_states.adjustment.normalize_to_monitor.wavelength_low, low)
        self.assertEqual(model.all_states.adjustment.normalize_to_monitor.wavelength_high, high)
        self.assertEqual(model.all_states.adjustment.normalize_to_monitor.wavelength_step, step)
        self.assertEqual(model.all_states.adjustment.normalize_to_monitor.wavelength_step_type, step_type)
        # Wavelength and pixel adjustment
        self.assertEqual(model.all_states.adjustment.wavelength_and_pixel_adjustment.wavelength_low, low)
        self.assertEqual(model.all_states.adjustment.wavelength_and_pixel_adjustment.wavelength_high, high)
        self.assertEqual(model.all_states.adjustment.wavelength_and_pixel_adjustment.wavelength_step, step)
        self.assertEqual(model.all_states.adjustment.wavelength_and_pixel_adjustment.wavelength_step_type, step_type)
        # Wavelength
        self.assertEqual(model.all_states.wavelength.wavelength_low, low)
        self.assertEqual(model.all_states.wavelength.wavelength_high, high)
        self.assertEqual(model.all_states.wavelength.wavelength_step, step)
        self.assertEqual(model.all_states.wavelength.wavelength_step_type, step_type)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_absolute_scale_has_an_empty_default_value(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(not state_gui_model.absolute_scale)

    def test_that_can_set_absolute_scale(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.absolute_scale = .5
        self.assertEqual(state_gui_model.absolute_scale, .5)

    def test_that_default_extents_are_empty(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(not state_gui_model.sample_width)
        self.assertTrue(not state_gui_model.sample_height)
        self.assertTrue(not state_gui_model.sample_thickness)
        self.assertTrue(not state_gui_model.z_offset)

    def test_that_default_sample_shape_is_cylinder_axis_up(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.sample_shape, None)

    def test_that_can_set_the_sample_geometry(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.sample_width = 1.2
        state_gui_model.sample_height = 1.6
        state_gui_model.sample_thickness = 1.8
        state_gui_model.z_offset = 1.78
        state_gui_model.sample_shape = SampleShape.FLAT_PLATE
        self.assertEqual(state_gui_model.sample_width, 1.2)
        self.assertEqual(state_gui_model.sample_height, 1.6)
        self.assertEqual(state_gui_model.sample_thickness, 1.8)
        self.assertEqual(state_gui_model.z_offset, 1.78)
        self.assertEqual(state_gui_model.sample_shape, SampleShape.FLAT_PLATE)

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q limits
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_limits_default_to_empty(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.q_1d_rebin_string, "")

        self.assertEqual(state_gui_model.q_xy_max, "")
        self.assertEqual(state_gui_model.q_xy_step, "")
        self.assertEqual(state_gui_model.q_xy_step_type, None)
        self.assertEqual(state_gui_model.r_cut, 0.0)
        self.assertEqual(state_gui_model.w_cut, 0.0)

    def test_that_can_set_the_q_limits(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.q_1d_rebin_string = "test"
        state_gui_model.q_xy_max = 1.
        state_gui_model.q_xy_step = 122.
        state_gui_model.q_xy_step_type = RangeStepType.LOG
        state_gui_model.r_cut = 45.
        state_gui_model.w_cut = 890.

        self.assertEqual(state_gui_model.q_1d_rebin_string, "test")
        self.assertEqual(state_gui_model.q_xy_max, 1.)
        self.assertEqual(state_gui_model.q_xy_step, 122.)
        self.assertEqual(state_gui_model.q_xy_step_type, RangeStepType.LOG)
        self.assertEqual(state_gui_model.r_cut, 45.)
        self.assertEqual(state_gui_model.w_cut, 890.)

    def test_that_q_1d_rebin_string_as_bytes_is_converted_to_string(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.q_1d_rebin_string = b"test"

        q_1d_rebin_string = state_gui_model.q_1d_rebin_string
        self.assertEqual(type(q_1d_rebin_string), bytes)
        self.assertEqual(q_1d_rebin_string, b"test")

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_gravity_extra_length_empty_by_default_and_usage_true_by_default(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertTrue(state_gui_model.gravity_on_off)
        self.assertEqual(state_gui_model.gravity_extra_length, 0.0)

    def test_that_can_set_gravity(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.gravity_on_off = False
        state_gui_model.gravity_extra_length = 1.
        self.assertFalse(state_gui_model.gravity_on_off)
        self.assertEqual(state_gui_model.gravity_extra_length, 1.)

    # ------------------------------------------------------------------------------------------------------------------
    # Q resolution
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_resolution_settings_show_empty_defaults(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertFalse(state_gui_model.use_q_resolution)
        self.assertEqual(state_gui_model.q_resolution_source_a, 0.0)
        self.assertEqual(state_gui_model.q_resolution_sample_a, 0.0)
        self.assertEqual(state_gui_model.q_resolution_source_h, 0.0)
        self.assertEqual(state_gui_model.q_resolution_sample_h, 0.0)
        self.assertEqual(state_gui_model.q_resolution_source_w, 0.0)
        self.assertEqual(state_gui_model.q_resolution_sample_w, 0.0)
        self.assertEqual(state_gui_model.q_resolution_collimation_length, "")
        self.assertEqual(state_gui_model.q_resolution_delta_r, 0.0)
        self.assertEqual(state_gui_model.q_resolution_moderator_file, "")

    def test_that_q_resolution_can_be_set_correctly(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.use_q_resolution = True
        state_gui_model.q_resolution_source_a = 1.5
        state_gui_model.q_resolution_sample_a = 2.5
        state_gui_model.q_resolution_source_h = 1.5
        state_gui_model.q_resolution_sample_h = 2.5
        state_gui_model.q_resolution_source_w = 1.5
        state_gui_model.q_resolution_sample_w = 2.5
        state_gui_model.q_resolution_collimation_length = 1.7
        state_gui_model.q_resolution_delta_r = 12.4
        state_gui_model.q_resolution_moderator_file = "test.txt"

        self.assertTrue(state_gui_model.use_q_resolution)
        self.assertEqual(state_gui_model.q_resolution_source_a, 1.5)
        self.assertEqual(state_gui_model.q_resolution_sample_a, 2.5)
        self.assertEqual(state_gui_model.q_resolution_source_h, 1.5)
        self.assertEqual(state_gui_model.q_resolution_sample_h, 2.5)
        self.assertEqual(state_gui_model.q_resolution_source_w, 1.5)
        self.assertEqual(state_gui_model.q_resolution_sample_w, 2.5)
        self.assertEqual(state_gui_model.q_resolution_collimation_length, 1.7)
        self.assertEqual(state_gui_model.q_resolution_delta_r, 12.4)
        self.assertEqual(state_gui_model.q_resolution_moderator_file, "test.txt")

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi mask
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_phi_mask_defaults_to_90_and_true_for_use_mirror(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.phi_limit_min, -90.0)
        self.assertEqual(state_gui_model.phi_limit_max, 90.0)
        self.assertTrue(state_gui_model.phi_limit_use_mirror)

    def test_that_phi_mask_can_be_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.phi_limit_min = 12.
        state_gui_model.phi_limit_max = 13.
        state_gui_model.phi_limit_use_mirror = True
        self.assertEqual(state_gui_model.phi_limit_min, 12.)
        self.assertEqual(state_gui_model.phi_limit_max, 13.)
        self.assertTrue(state_gui_model.phi_limit_use_mirror)

    # ------------------------------------------------------------------------------------------------------------------
    # Radius mask
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_radius_mask_defaults_to_empty(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.radius_limit_min, 0.0)
        self.assertEqual(state_gui_model.radius_limit_max, 0.0)

    def test_that_radius_mask_can_be_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.radius_limit_min = 12.
        state_gui_model.radius_limit_max = 13.
        self.assertEqual(state_gui_model.radius_limit_min, 12.)
        self.assertEqual(state_gui_model.radius_limit_max, 13.)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_mask_file_defaults_are_empty(self):
        state_gui_model = StateGuiModel(AllStates())
        self.assertEqual(state_gui_model.mask_files, [])

    def test_that_mask_file_can_be_set(self):
        state_gui_model = StateGuiModel(AllStates())
        state_gui_model.mask_files = ["file.txt", "file2.txt"]
        self.assertEqual(state_gui_model.mask_files, ["file.txt", "file2.txt"])

    # ------------------------------------------------------------------------------------------------------------------
    # User files - focus on fields that are displayed in mm and stored in m
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_user_file_items_interpreted_correctly(self):
        state = AllStates()
        state.move.sample_offset = 1.78 / 1000.
        state.scale.width = 1.2
        state.scale.height = 1.6
        state.scale.thickness = 1.8
        state.scale.shape = SampleShape.FLAT_PLATE
        state.convert_to_q.radius_cutoff = 45. / 1000.
        state.convert_to_q.q_resolution_a1 = 1.5 / 1000.
        state.convert_to_q.q_resolution_a2 = 2.5 / 1000.
        state.convert_to_q.q_resolution_h1 = 1.5 / 1000.
        state.convert_to_q.q_resolution_h2 = 2.5 / 1000.
        state.convert_to_q.q_resolution_delta_r = 0.1 / 1000.
        state.mask.radius_min = 12. / 1000.
        state.mask.radius_max = 13. / 1000.
        state.move.detectors = {DetectorType.LAB.value: StateMoveDetectors(),
                                DetectorType.HAB.value: StateMoveDetectors()}
        state.move.detectors[DetectorType.LAB.value].sample_centre_pos1 = 21.5 / 1000.
        state.move.detectors[DetectorType.LAB.value].sample_centre_pos2 = 17.8 / 1000.
        state.move.detectors[DetectorType.HAB.value].sample_centre_pos1 = 25.1 / 1000.
        state.move.detectors[DetectorType.HAB.value].sample_centre_pos2 = 16.9 / 1000.
        state_gui_model = StateGuiModel(state)
        self.assertEqual(state_gui_model.sample_width, 1.2)
        self.assertEqual(state_gui_model.sample_height, 1.6)
        self.assertEqual(state_gui_model.sample_thickness, 1.8)
        self.assertEqual(state_gui_model.z_offset, 1.78)
        self.assertEqual(state_gui_model.sample_shape, SampleShape.FLAT_PLATE)
        self.assertEqual(state_gui_model.r_cut, 45.)
        self.assertEqual(state_gui_model.q_resolution_source_a, 1.5)
        self.assertEqual(state_gui_model.q_resolution_sample_a, 2.5)
        self.assertEqual(state_gui_model.q_resolution_source_h, 1.5)
        self.assertEqual(state_gui_model.q_resolution_sample_h, 2.5)
        self.assertEqual(state_gui_model.q_resolution_delta_r, 0.1)
        self.assertEqual(state_gui_model.radius_limit_min, 12.)
        self.assertEqual(state_gui_model.radius_limit_max, 13.)
        self.assertEqual(state_gui_model.lab_pos_1, 21.5)
        self.assertEqual(state_gui_model.lab_pos_2, 17.8)
        self.assertEqual(state_gui_model.hab_pos_1, 25.1)
        self.assertEqual(state_gui_model.hab_pos_2, 16.9)


if __name__ == '__main__':
    unittest.main()
