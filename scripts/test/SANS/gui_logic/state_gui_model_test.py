from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.user_file.user_file_common import (OtherId, event_binning_string_values, DetectorId)
from sans.common.enums import (ReductionDimensionality, ISISReductionMode, RangeStepType, SampleShape, SaveType)


class StateGuiModelTest(unittest.TestCase):
    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_default_compatibility_mode_is_false(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertFalse(state_gui_model.compatibility_mode)

    def test_that_can_set_compatibility_mode(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.compatibility_mode = True
        self.assertTrue(state_gui_model.compatibility_mode)

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
        self.assertTrue(state_gui_model.save_types == [SaveType.NXcanSAS])

    def test_that_can_select_multiple_save_types(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.save_types = [SaveType.RKH, SaveType.NXcanSAS]
        self.assertTrue(state_gui_model.save_types == [SaveType.RKH, SaveType.NXcanSAS])

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
        self.assertTrue(state_gui_model.event_slices == "")

    def test_that_slice_event_can_be_retrieved_if_it_exists(self):
        state_gui_model = StateGuiModel({OtherId.event_slices: [event_binning_string_values(value="test")]})
        self.assertTrue(state_gui_model.event_slices == "test")

    def test_that_slice_event_can_be_updated(self):
        state_gui_model = StateGuiModel({OtherId.event_slices: [event_binning_string_values(value="test")]})
        state_gui_model.event_slices = "test2"
        self.assertTrue(state_gui_model.event_slices == "test2")

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_1D_reduction_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.reduction_dimensionality is ReductionDimensionality.OneDim)

    def test_that_is_set_to_2D_reduction(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.reduction_dimensionality = ReductionDimensionality.TwoDim
        self.assertTrue(state_gui_model.reduction_dimensionality is ReductionDimensionality.TwoDim)

    def test_that_raises_when_not_setting_with_reduction_dim_enum(self):
        def red_dim_wrapper():
            state_gui_model = StateGuiModel({"test": [1]})
            state_gui_model.reduction_dimensionality = "string"
        self.assertRaises(ValueError, red_dim_wrapper)

    def test_that_can_update_reduction_dimensionality(self):
        state_gui_model = StateGuiModel({OtherId.reduction_dimensionality: [ReductionDimensionality.OneDim]})
        self.assertTrue(state_gui_model.reduction_dimensionality is ReductionDimensionality.OneDim)
        state_gui_model.reduction_dimensionality = ReductionDimensionality.TwoDim
        self.assertTrue(state_gui_model.reduction_dimensionality is ReductionDimensionality.TwoDim)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction mode
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_is_set_to_lab_by_default(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.reduction_mode is ISISReductionMode.LAB)

    def test_that_can_be_set_to_something_else(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.reduction_mode = ISISReductionMode.Merged
        self.assertTrue(state_gui_model.reduction_mode is ISISReductionMode.Merged)

    def test_that_raises_when_setting_with_wrong_input(self):
        def red_mode_wrapper():
            state_gui_model = StateGuiModel({"test": [1]})
            state_gui_model.reduction_mode = "string"
        self.assertRaises(ValueError, red_mode_wrapper)

    def test_that_can_update_reduction_mode(self):
        state_gui_model = StateGuiModel({DetectorId.reduction_mode: [ISISReductionMode.HAB]})
        self.assertTrue(state_gui_model.reduction_mode is ISISReductionMode.HAB)
        state_gui_model.reduction_mode = ISISReductionMode.All
        self.assertTrue(state_gui_model.reduction_mode is ISISReductionMode.All)

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
        self.assertTrue(state_gui_model.wavelength_step_type is RangeStepType.Lin)

    def test_that_can_set_wavelength(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.wavelength_min = 1.
        state_gui_model.wavelength_max = 2.
        state_gui_model.wavelength_step = .5
        state_gui_model.wavelength_step_type = RangeStepType.Lin
        state_gui_model.wavelength_step_type = RangeStepType.Log
        self.assertTrue(state_gui_model.wavelength_min == 1.)
        self.assertTrue(state_gui_model.wavelength_max == 2.)
        self.assertTrue(state_gui_model.wavelength_step == .5)
        self.assertTrue(state_gui_model.wavelength_step_type is RangeStepType.Log)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_absolute_scale_has_an_empty_default_value(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(not state_gui_model.absolute_scale)

    def test_that_can_set_absolute_scale(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.absolute_scale = .5
        self.assertTrue(state_gui_model.absolute_scale == .5)

    def test_that_default_extents_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(not state_gui_model.sample_width)
        self.assertTrue(not state_gui_model.sample_height)
        self.assertTrue(not state_gui_model.sample_thickness)
        self.assertTrue(not state_gui_model.z_offset)

    def test_that_default_sample_shape_is_cylinder_axis_up(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.sample_shape is None)

    def test_that_can_set_the_sample_geometry(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.sample_width = 1.2
        state_gui_model.sample_height = 1.6
        state_gui_model.sample_thickness = 1.8
        state_gui_model.z_offset = 1.78
        state_gui_model.sample_shape = SampleShape.Cuboid
        self.assertTrue(state_gui_model.sample_width == 1.2)
        self.assertTrue(state_gui_model.sample_height == 1.6)
        self.assertTrue(state_gui_model.sample_thickness == 1.8)
        self.assertTrue(state_gui_model.z_offset == 1.78)
        self.assertTrue(state_gui_model.sample_shape is SampleShape.Cuboid)

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
        self.assertTrue(state_gui_model.normalization_incident_monitor == "")
        self.assertFalse(state_gui_model.normalization_interpolate)

    def test_that_can_set_normalize_to_monitor(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.normalization_incident_monitor = 2
        state_gui_model.normalization_interpolate = True
        self.assertTrue(state_gui_model.normalization_incident_monitor == 2)
        self.assertTrue(state_gui_model.normalization_interpolate)
        # Reassign
        state_gui_model.normalization_incident_monitor = 3
        self.assertTrue(state_gui_model.normalization_incident_monitor == 3)

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission
    # ------------------------------------------------------------------------------------------------------------------
    def test_that_transmission_monitor_defaults_are_empty_and_false_for_interpolating_rebin(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.transmission_incident_monitor == "")
        self.assertFalse(state_gui_model.transmission_interpolate)

    def test_that_can_set_transmission_monitor(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_incident_monitor = 2
        state_gui_model.transmission_interpolate = True
        self.assertTrue(state_gui_model.transmission_incident_monitor == 2)
        self.assertTrue(state_gui_model.transmission_interpolate)
        # # Reassign
        state_gui_model.transmission_incident_monitor = 3
        self.assertTrue(state_gui_model.transmission_incident_monitor == 3)

    def test_that_can_set_normalization_and_transmission_monitor_and_rebin_type_settings(self):
        pass

    def test_that_the_default_transmission_roi_and_mask_files_and_radius_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.transmission_roi_files == "")
        self.assertTrue(state_gui_model.transmission_mask_files == "")
        self.assertTrue(state_gui_model.transmission_radius == "")

    def test_that_can_set_transmission_roi_mask_and_radius(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_roi_files = "roi.xml"
        state_gui_model.transmission_mask_files = "mask.xml"
        state_gui_model.transmission_radius = 8.
        self.assertTrue(state_gui_model.transmission_roi_files == "roi.xml")
        self.assertTrue(state_gui_model.transmission_mask_files == "mask.xml")
        self.assertTrue(state_gui_model.transmission_radius == 8)

    def test_that_default_transmission_monitor_is_3(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.transmission_monitor == 3)

    def test_that_transmission_monitor_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_monitor = 4
        self.assertTrue(state_gui_model.transmission_monitor == 4)

    def test_that_transmission_m4_shift_default_is_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.transmission_m4_shift == "")

    def test_that_transmission_m4_shift_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.transmission_m4_shift = 234
        self.assertTrue(state_gui_model.transmission_m4_shift == 234)

    def test_that_default_for_adjustment_files_are_empty(self):
        state_gui_model = StateGuiModel({"test": [1]})
        self.assertTrue(state_gui_model.wavelength_adjustment_det_1 == "")
        self.assertTrue(state_gui_model.wavelength_adjustment_det_2 == "")
        self.assertTrue(state_gui_model.pixel_adjustment_det_1 == "")
        self.assertTrue(state_gui_model.pixel_adjustment_det_2 == "")

    def test_that_adjustment_files_can_be_set(self):
        state_gui_model = StateGuiModel({"test": [1]})
        state_gui_model.wavelength_adjustment_det_1 = "wav1.txt"
        state_gui_model.wavelength_adjustment_det_2 = "wav2.txt"
        state_gui_model.pixel_adjustment_det_1 = "pix1.txt"
        state_gui_model.pixel_adjustment_det_2 = "pix2.txt"
        self.assertTrue(state_gui_model.wavelength_adjustment_det_1 == "wav1.txt")
        self.assertTrue(state_gui_model.wavelength_adjustment_det_2 == "wav2.txt")
        self.assertTrue(state_gui_model.pixel_adjustment_det_1 == "pix1.txt")
        self.assertTrue(state_gui_model.pixel_adjustment_det_2 == "pix2.txt")


if __name__ == '__main__':
    unittest.main()


