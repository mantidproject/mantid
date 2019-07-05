# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.user_file.settings_tags import (OtherId, event_binning_string_values, DetectorId, det_fit_range)
from sans.common.enums import (ReductionDimensionality, ISISReductionMode, RangeStepType, SampleShape, SaveType,
                               FitType, SANSInstrument)
from sans.user_file.settings_tags import (det_fit_range)


class StateGuiModelTest(unittest.TestCase):
    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================
    def test_that_default_instrument_is_NoInstrument(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.instrument, SANSInstrument.NoInstrument)

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_compatibility_mode_is_true(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.compatibility_mode)

    def test_that_can_set_compatibility_mode(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.compatibility_mode = False
        self.assertFalse(state_gui_model.compatibility_mode)

    def test_that_default_event_slice_optimisation_is_false(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertFalse(state_gui_model.event_slice_optimisation)

    def test_that_can_set_event_slice_optimisation(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.event_slice_optimisation = True
        self.assertTrue(state_gui_model.event_slice_optimisation)

    # ------------------------------------------------------------------------------------------------------------------
    # Save options
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_can_zero_error_free_saving_is_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.zero_error_free)

    def test_that_can_zero_error_free_saving_can_be_changed(self):
        state_gui_model = StateGuiModel({OtherId.save_as_zero_error_free: [True]})
        state_gui_model.zero_error_free = False
        self.assertFalse(state_gui_model.zero_error_free)

    def test_that_default_save_type_is_NXcanSAS(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.save_types, [SaveType.NXcanSAS])

    def test_that_can_select_multiple_save_types(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.save_types = [SaveType.RKH, SaveType.NXcanSAS]
        self.assertEqual(state_gui_model.save_types, [SaveType.RKH, SaveType.NXcanSAS])

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Event slices
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_if_no_slice_event_is_present_an_empty_string_is_returned(self):
        state_gui_model = StateGuiModel({"test": 1})
        self.assertEqual(state_gui_model.event_slices, "")

    def test_that_slice_event_can_be_retrieved_if_it_exists(self):
        state_gui_model = StateGuiModel({OtherId.event_slices: [event_binning_string_values(value="test")]})
        self.assertEqual(state_gui_model.event_slices, "test")

    def test_that_slice_event_can_be_updated(self):
        state_gui_model = StateGuiModel({OtherId.event_slices: [event_binning_string_values(value="test")]})
        state_gui_model.event_slices = "test2"
        self.assertEqual(state_gui_model.event_slices, "test2")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_1D_reduction_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.OneDim)

    def test_that_is_set_to_2D_reduction(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.reduction_dimensionality = ReductionDimensionality.TwoDim
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.TwoDim)

    def test_that_raises_when_not_setting_with_reduction_dim_enum(self):
        def red_dim_wrapper():
            state_gui_model = StateGuiModel({"test": [1]})
            state_gui_model.reduction_dimensionality = "string"
        self.assertRaises(ValueError, red_dim_wrapper)

    def test_that_can_update_reduction_dimensionality(self):
        state_gui_model = StateGuiModel({OtherId.reduction_dimensionality: [ReductionDimensionality.OneDim]})
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.OneDim)
        state_gui_model.reduction_dimensionality = ReductionDimensionality.TwoDim
        self.assertEqual(state_gui_model.reduction_dimensionality, ReductionDimensionality.TwoDim)

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_event_binning_default_settings_are_emtpy(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.event_binning, "")

    def test_that_event_binning_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.event_binning = "1,-1,10"
        self.assertEqual(state_gui_model.event_binning, "1,-1,10")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_set_to_lab_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.reduction_mode, ISISReductionMode.LAB)

    def test_that_can_be_set_to_something_else(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.reduction_mode = ISISReductionMode.Merged
        self.assertEqual(state_gui_model.reduction_mode, ISISReductionMode.Merged)

    def test_that_raises_when_setting_with_wrong_input(self):
        def red_mode_wrapper():
            state_gui_model = StateGuiModel({"test": [1]})
            state_gui_model.reduction_mode = "string"
        self.assertRaises(ValueError, red_mode_wrapper)

    def test_that_can_update_reduction_mode(self):
        state_gui_model = StateGuiModel({DetectorId.reduction_mode: [ISISReductionMode.HAB]})
        self.assertEqual(state_gui_model.reduction_mode, ISISReductionMode.HAB)
        state_gui_model.reduction_mode = ISISReductionMode.All
        self.assertEqual(state_gui_model.reduction_mode, ISISReductionMode.All)

    # ------------------------------------------------------------------------------------------------------------------
    # Merge range
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_merge_mask_is_false_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertFalse(state_gui_model.merge_mask)

    def test_that_merge_mask_can_be_set_to_something_else(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.merge_mask = True
        self.assertTrue(state_gui_model.merge_mask)

    def test_that_merge_min_is_None_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.merge_min, None)

    def test_that_merge_max_is_None_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.merge_max, None)

    def test_that_merge_min_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.merge_min = 78.9
        self.assertEqual(state_gui_model.merge_min, 78.9)

    def test_that_merge_range_set_correctly(self):
        state_gui_model = StateGuiModel({DetectorId.merge_range: [det_fit_range(use_fit=True, start=0.13, stop=0.15)]})
        self.assertEqual(state_gui_model.merge_min, 0.13)
        self.assertEqual(state_gui_model.merge_max, 0.15)
        self.assertTrue(state_gui_model.merge_mask)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_defaults_for_merge_are_empty_and_false(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.merge_scale, "1.0")
        self.assertEqual(state_gui_model.merge_shift, "0.0")
        self.assertFalse(state_gui_model.merge_scale_fit)
        self.assertFalse(state_gui_model.merge_shift_fit)
        self.assertEqual(state_gui_model.merge_q_range_start, "")
        self.assertEqual(state_gui_model.merge_q_range_stop, "")

    def test_that_can_set_and_reset_merged_settings(self):
        state_gui_model = StateGuiModel({DetectorId.shift_fit: [det_fit_range(start=1., stop=2., use_fit=True)],
                                         DetectorId.rescale_fit: [det_fit_range(start=1.4, stop=7., use_fit=False)],
                                         DetectorId.rescale: [12.],
                                         DetectorId.shift: [234.]})
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
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(not state_gui_model.wavelength_min)
        self.assertTrue(not state_gui_model.wavelength_max)
        self.assertTrue(not state_gui_model.wavelength_step)

    def test_that_default_wavelength_step_type_is_linear(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.wavelength_step_type,  RangeStepType.Lin)

    def test_that_can_set_wavelength(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.wavelength_min = 1.
        state_gui_model.wavelength_max = 2.
        state_gui_model.wavelength_step = .5
        state_gui_model.wavelength_step_type = RangeStepType.Lin
        state_gui_model.wavelength_step_type = RangeStepType.Log
        self.assertEqual(state_gui_model.wavelength_min, 1.)
        self.assertEqual(state_gui_model.wavelength_max, 2.)
        self.assertEqual(state_gui_model.wavelength_step, .5)
        self.assertEqual(state_gui_model.wavelength_step_type, RangeStepType.Log)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_absolute_scale_has_an_empty_default_value(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(not state_gui_model.absolute_scale)

    def test_that_can_set_absolute_scale(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.absolute_scale = .5
        self.assertEqual(state_gui_model.absolute_scale, .5)

    def test_that_default_extents_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(not state_gui_model.sample_width)
        self.assertTrue(not state_gui_model.sample_height)
        self.assertTrue(not state_gui_model.sample_thickness)
        self.assertTrue(not state_gui_model.z_offset)

    def test_that_default_sample_shape_is_cylinder_axis_up(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.sample_shape, None)

    def test_that_can_set_the_sample_geometry(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.sample_width = 1.2
        state_gui_model.sample_height = 1.6
        state_gui_model.sample_thickness = 1.8
        state_gui_model.z_offset = 1.78
        state_gui_model.sample_shape = SampleShape.FlatPlate
        self.assertEqual(state_gui_model.sample_width, 1.2)
        self.assertEqual(state_gui_model.sample_height, 1.6)
        self.assertEqual(state_gui_model.sample_thickness, 1.8)
        self.assertEqual(state_gui_model.z_offset, 1.78)
        self.assertEqual(state_gui_model.sample_shape, SampleShape.FlatPlate)

    # ==================================================================================================================
    # ==================================================================================================================
    # ADJUSTMENT TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Normalize to monitor
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_normalize_to_monitor_defaults_are_empty_for_monitor_and_false_for_interpolating_rebin(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.normalization_incident_monitor, "")
        self.assertFalse(state_gui_model.normalization_interpolate)

    def test_that_can_set_normalize_to_monitor(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.normalization_incident_monitor = 2
        state_gui_model.normalization_interpolate = True
        self.assertEqual(state_gui_model.normalization_incident_monitor, 2)
        self.assertTrue(state_gui_model.normalization_interpolate)
        # Reassign
        state_gui_model.normalization_incident_monitor = 3
        self.assertEqual(state_gui_model.normalization_incident_monitor, 3)

    def test_that_can_set_only_interpolation(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.normalization_interpolate = True
        self.assertEqual(state_gui_model.normalization_incident_monitor, None)
        self.assertTrue(state_gui_model.normalization_interpolate)

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_transmission_monitor_defaults_are_empty_and_false_for_interpolating_rebin(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_incident_monitor, "")
        self.assertFalse(state_gui_model.transmission_interpolate)

    def test_that_can_set_transmission_monitor(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_incident_monitor = 2
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, 2)
        self.assertTrue(state_gui_model.transmission_interpolate)
        # # Reassign
        state_gui_model.transmission_incident_monitor = 3
        self.assertEqual(state_gui_model.transmission_incident_monitor, 3)

    def test_that_can_set_only_transmission_interpolation(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, None)
        self.assertTrue(state_gui_model.transmission_interpolate)

    def test_that_can_set_normalization_and_transmission_monitor_and_rebin_type_settings(self):
        pass

    def test_that_the_default_transmission_roi_and_mask_files_and_radius_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_roi_files, "")
        self.assertEqual(state_gui_model.transmission_mask_files, "")
        self.assertEqual(state_gui_model.transmission_radius, "")

    def test_that_can_set_transmission_roi_mask_and_radius(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_roi_files = "roi.xml"
        state_gui_model.transmission_mask_files = "mask.xml"
        state_gui_model.transmission_radius = 8.
        self.assertEqual(state_gui_model.transmission_roi_files, "roi.xml")
        self.assertEqual(state_gui_model.transmission_mask_files, "mask.xml")
        self.assertEqual(state_gui_model.transmission_radius, 8)

    def test_that_default_transmission_monitor_is_3(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_monitor, 3)

    def test_that_transmission_monitor_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_monitor = 4
        self.assertEqual(state_gui_model.transmission_monitor, 4)

    def test_that_transmission_mn_shift_default_is_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_mn_shift, "")

    def test_that_transmission_mn_shift_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_mn_shift = 234
        self.assertEqual(state_gui_model.transmission_mn_shift, 234)

    def test_that_default_for_adjustment_files_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "")

    def test_that_adjustment_files_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.wavelength_adjustment_det_1 = "wav1.txt"
        state_gui_model.wavelength_adjustment_det_2 = "wav2.txt"
        state_gui_model.pixel_adjustment_det_1 = "pix1.txt"
        state_gui_model.pixel_adjustment_det_2 = "pix2.txt"
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "wav1.txt")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "wav2.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "pix1.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "pix2.txt")

    def test_transmission_fit_defaults(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.NoFit)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.NoFit)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 2)

    def test_that_can_set_transmission_fit_options(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_sample_fit_type = FitType.Logarithmic
        state_gui_model.transmission_can_fit_type = FitType.Linear
        state_gui_model.transmission_sample_polynomial_order = 2
        state_gui_model.transmission_can_polynomial_order = 2
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.Logarithmic)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.Linear)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 2)

    def test_that_transmission_fit_wavelength_defaults_to_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, "")

    def test_that_transmission_fit_wavelength_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_sample_wavelength_min = 1.3
        state_gui_model.transmission_sample_wavelength_max = 10.3
        state_gui_model.transmission_can_wavelength_min = 1.3
        state_gui_model.transmission_can_wavelength_max = 10.3

        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, 10.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, 10.3)

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q limits
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_limits_default_to_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.q_1d_rebin_string, "")

        self.assertEqual(state_gui_model.q_xy_max, "")
        self.assertEqual(state_gui_model.q_xy_step, "")
        self.assertEqual(state_gui_model.q_xy_step_type, None)
        self.assertEqual(state_gui_model.r_cut, "")
        self.assertEqual(state_gui_model.w_cut, "")

    def test_that_can_set_the_q_limits(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.q_1d_rebin_string = "test"
        state_gui_model.q_xy_max = 1.
        state_gui_model.q_xy_step = 122.
        state_gui_model.q_xy_step_type = RangeStepType.Log
        state_gui_model.r_cut = 45.
        state_gui_model.w_cut = 890.

        self.assertEqual(state_gui_model.q_1d_rebin_string, "test")
        self.assertEqual(state_gui_model.q_xy_max, 1.)
        self.assertEqual(state_gui_model.q_xy_step, 122.)
        self.assertEqual(state_gui_model.q_xy_step_type, RangeStepType.Log)
        self.assertEqual(state_gui_model.r_cut, 45.)
        self.assertEqual(state_gui_model.w_cut, 890.)

    def test_that_q_1d_rebin_string_as_bytes_is_converted_to_string(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.q_1d_rebin_string = b"test"

        q_1d_rebin_string = state_gui_model.q_1d_rebin_string
        self.assertEqual(type(q_1d_rebin_string), str)
        self.assertEqual(q_1d_rebin_string, "test")

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_gravity_extra_length_empty_by_default_and_usage_true_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.gravity_on_off)
        self.assertEqual(state_gui_model.gravity_extra_length, "")

    def test_that_can_set_gravity(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.gravity_on_off = False
        state_gui_model.gravity_extra_length = 1.
        self.assertFalse(state_gui_model.gravity_on_off)
        self.assertEqual(state_gui_model.gravity_extra_length, 1.)

    # ------------------------------------------------------------------------------------------------------------------
    # Q resolution
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_q_resolution_settings_show_empty_defaults(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertFalse(state_gui_model.use_q_resolution)
        self.assertEqual(state_gui_model.q_resolution_source_a, "")
        self.assertEqual(state_gui_model.q_resolution_sample_a, "")
        self.assertEqual(state_gui_model.q_resolution_source_h, "")
        self.assertEqual(state_gui_model.q_resolution_sample_h, "")
        self.assertEqual(state_gui_model.q_resolution_source_w, "")
        self.assertEqual(state_gui_model.q_resolution_sample_w, "")
        self.assertEqual(state_gui_model.q_resolution_collimation_length, "")
        self.assertEqual(state_gui_model.q_resolution_delta_r, "")
        self.assertEqual(state_gui_model.q_resolution_moderator_file, "")

    def test_that_q_resolution_can_be_set_correctly(self):
        state_gui_model = StateGuiModel({"test": [1]})
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
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.phi_limit_min, "-90")
        self.assertEqual(state_gui_model.phi_limit_max, "90")
        self.assertTrue(state_gui_model.phi_limit_use_mirror)

    def test_that_phi_mask_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
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
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.radius_limit_min, "")
        self.assertEqual(state_gui_model.radius_limit_max, "")

    def test_that_radius_mask_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.radius_limit_min = 12.
        state_gui_model.radius_limit_max = 13.
        self.assertEqual(state_gui_model.radius_limit_min, 12.)
        self.assertEqual(state_gui_model.radius_limit_max, 13.)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_mask_file_defaults_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertEqual(state_gui_model.mask_files, [])

    def test_that_mask_file_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.mask_files = ["file.txt", "file2.txt"]
        self.assertEqual(state_gui_model.mask_files, ["file.txt", "file2.txt"])


if __name__ == '__main__':
    unittest.main()
