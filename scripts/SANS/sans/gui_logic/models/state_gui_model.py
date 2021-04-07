# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
""" The state gui model contains all the reduction information which is not explicitly available in the data table.

This is one of the two models which is used for the data reduction. It contains generally all the settings which
are not available in the model associated with the data table.
"""

from sans.common.enums import (ReductionDimensionality, ReductionMode, RangeStepType, SaveType,
                               DetectorType, FitModeForMerge)
from sans.common.general_functions import get_ranges_from_event_slice_setting
from sans.gui_logic.models.model_common import ModelCommon
from sans.state.AllStates import AllStates
from sans.gui_logic.gui_common import (meter_2_millimeter, millimeter_2_meter)
from sans.user_file.parser_helpers.wavelength_parser import parse_range_wavelength


class StateGuiModel(ModelCommon):
    def __init__(self, all_states: AllStates):
        assert(isinstance(all_states, AllStates)), \
            "Expected AllStates, got %r, could be a legacy API caller" % repr(all_states)
        super(StateGuiModel, self).__init__(all_states)

        # This is transformed in State* objects so we need the
        # unparsed string for the GUI to display
        self._wavelength_range = ''

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    @property
    def all_states(self) -> AllStates:
        return self._all_states

    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode Options
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def compatibility_mode(self):
        val = self._all_states.compatibility.use_compatibility_mode
        return self._get_val_or_default(val, True)

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self._all_states.compatibility.use_compatibility_mode = value

    @property
    def event_slice_optimisation(self):
        val = self._all_states.compatibility.use_event_slice_optimisation
        return self._get_val_or_default(val, False)

    @event_slice_optimisation.setter
    def event_slice_optimisation(self, value):
        self._all_states.compatibility.use_event_slice_optimisation = value

    # ------------------------------------------------------------------------------------------------------------------
    # Save Options
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def zero_error_free(self):
        val = self._all_states.save.zero_free_correction
        # Default on
        return self._get_val_or_default(val, True)

    @zero_error_free.setter
    def zero_error_free(self, value):
        self._all_states.save.zero_free_correction = value

    @property
    def save_types(self):
        val = self._all_states.save.file_format
        return self._get_val_or_default(val, [SaveType.NX_CAN_SAS])

    @save_types.setter
    def save_types(self, value):
        self._all_states.save.file_format = value

    @property
    def user_file(self):
        val = self._all_states.save.user_file
        return self._get_val_or_default(val)

    @user_file.setter
    def user_file(self, value):
        self._all_states.save.user_file = value

    @property
    def batch_file(self):
        val = self._all_states.save.batch_file
        return self._get_val_or_default(val)

    @batch_file.setter
    def batch_file(self, value):
        self._all_states.save.batch_file = value

    # ==================================================================================================================
    # ==================================================================================================================
    # BeamCentre TAB
    # ==================================================================================================================
    # ==================================================================================================================
    @property
    def lab_pos_1(self):
        val = self._all_states.move.detectors[DetectorType.LAB.value].sample_centre_pos1
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self._all_states.move.detectors[DetectorType.LAB.value].sample_centre_pos1 = millimeter_2_meter(value)

    @property
    def lab_pos_2(self):
        val = self._all_states.move.detectors[DetectorType.LAB.value].sample_centre_pos2
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self._all_states.move.detectors[DetectorType.LAB.value].sample_centre_pos2 = millimeter_2_meter(value)

    @property
    def hab_pos_1(self):
        val = None
        if DetectorType.HAB.value in self._all_states.move.detectors:
            val = self._all_states.move.detectors[DetectorType.HAB.value].sample_centre_pos1
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        if DetectorType.HAB.value in self._all_states.move.detectors:
            self._all_states.move.detectors[DetectorType.HAB.value].sample_centre_pos1 = millimeter_2_meter(value)

    @property
    def hab_pos_2(self):
        val = None
        if DetectorType.HAB.value in self._all_states.move.detectors:
            val = self._all_states.move.detectors[DetectorType.HAB.value].sample_centre_pos2
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        if DetectorType.HAB.value in self._all_states.move.detectors:
            self._all_states.move.detectors[DetectorType.HAB.value].sample_centre_pos1 = millimeter_2_meter(value)

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Event slices
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_slices(self):
        val = self._all_states.slice.event_slice_str
        return self._get_val_or_default(val)

    @event_slices.setter
    def event_slices(self, value):
        self._all_states.slice.event_slice_str = value
        start, stop = get_ranges_from_event_slice_setting(value)
        self._all_states.slice.start_time = start
        self._all_states.slice.end_time = stop

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_dimensionality(self):
        val = self._all_states.reduction.reduction_dimensionality
        return self._get_val_or_default(val, ReductionDimensionality.ONE_DIM)

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value):
        if value is ReductionDimensionality.ONE_DIM or value is ReductionDimensionality.TWO_DIM:
            self._all_states.convert_to_q.reduction_dimensionality = value
            self._all_states.reduction.reduction_dimensionality = value
        else:
            raise ValueError("A reduction dimensionality was expected, got instead {}".format(value))

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction Mode
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_mode(self):
        val = self._all_states.reduction.reduction_mode
        return self._get_val_or_default(val, ReductionMode.LAB)

    @reduction_mode.setter
    def reduction_mode(self, value):
        if (value is ReductionMode.LAB or value is ReductionMode.HAB
                or value is ReductionMode.MERGED or value is ReductionMode.ALL):  # noqa
            self._all_states.reduction.reduction_mode = value
        else:
            raise ValueError("A reduction mode was expected, got instead {}".format(value))

    @property
    def merge_scale(self):
        val = self._all_states.reduction.merge_scale
        return self._get_val_or_default(val, "1.0")

    @merge_scale.setter
    def merge_scale(self, value):
        self._all_states.reduction.merge_scale = value

    @property
    def merge_shift(self):
        val = self._all_states.reduction.merge_shift
        return self._get_val_or_default(val, "0.0")

    @merge_shift.setter
    def merge_shift(self, value):
        self._all_states.reduction.merge_shift = value

    @property
    def merge_scale_fit(self):
        fit_mode = self._all_states.reduction.merge_fit_mode
        return fit_mode is FitModeForMerge.SCALE_ONLY or fit_mode is FitModeForMerge.BOTH

    @merge_scale_fit.setter
    def merge_scale_fit(self, value):
        if value:
            if self.merge_shift_fit:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.BOTH
            else:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.SCALE_ONLY
        else:
            if self.merge_shift_fit:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.SHIFT_ONLY
            else:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.NO_FIT

    @property
    def merge_shift_fit(self):
        fit_mode = self._all_states.reduction.merge_fit_mode
        return fit_mode is FitModeForMerge.SHIFT_ONLY or fit_mode is FitModeForMerge.BOTH

    @merge_shift_fit.setter
    def merge_shift_fit(self, value):
        if value:
            if self.merge_scale_fit:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.BOTH
            else:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.SHIFT_ONLY
        else:
            if self.merge_scale_fit:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.SCALE_ONLY
            else:
                self._all_states.reduction.merge_fit_mode = FitModeForMerge.NO_FIT

    @property
    def merge_q_range_start(self):
        val = self._all_states.reduction.merge_range_min
        return self._get_val_or_default(val)

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        self._all_states.reduction.merge_range_min = value

    @property
    def merge_q_range_stop(self):
        val = self._all_states.reduction.merge_range_max
        return self._get_val_or_default(val)

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        self._all_states.reduction.merge_range_max = value

    @property
    def merge_mask(self):
        val = self._all_states.reduction.merge_mask
        return self._get_val_or_default(val, False)

    @merge_mask.setter
    def merge_mask(self, value):
        self._all_states.reduction.merge_mask = value

    @property
    def merge_max(self):
        val = self._all_states.reduction.merge_max
        return self._get_val_or_default(val, None)

    @merge_max.setter
    def merge_max(self, value):
        self._all_states.reduction.merge_max = value

    @property
    def merge_min(self):
        val = self._all_states.reduction.merge_min
        return self._get_val_or_default(val, None)

    @merge_min.setter
    def merge_min(self, value):
        self._all_states.reduction.merge_min = value

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_binning(self):
        val = self._all_states.compatibility.time_rebin_string
        return self._get_val_or_default(val)

    @event_binning.setter
    def event_binning(self, value):
        self._all_states.compatibility.time_rebin_string = value

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength properties
    # Note that the wavelength settings are being used in four sub-states:
    # - wavelength
    # - calculate_transmission
    # - normalize_to_monitor
    # - wavelength_and_pixel_adjustment
    # --------------------------------------------------------------------------------------------------------------

    def _assert_all_wavelength_same(self, attr_name):
        # For god knows what reason we have the same data duplicated in 4 places
        # Ensure they stay in sync
        calc_trans = self._all_states.adjustment.calculate_transmission
        norm_mon = self._all_states.adjustment.normalize_to_monitor
        wav_pixel = self._all_states.adjustment.wavelength_and_pixel_adjustment
        wavelength = self._all_states.wavelength
        to_check = [getattr(calc_trans, attr_name), getattr(norm_mon, attr_name),
                    getattr(wav_pixel, attr_name), getattr(wavelength, attr_name)]
        assert all(x == to_check[0] for x in to_check), \
            "Wavelength attributes have got out of sync. This should not happen!"

    def _set_on_all_wavelength(self, attr_name, value):
        objs_to_set = [self._all_states.adjustment.calculate_transmission,
                       self._all_states.adjustment.normalize_to_monitor,
                       self._all_states.adjustment.wavelength_and_pixel_adjustment,
                       self._all_states.wavelength]
        for obj in objs_to_set:
            setattr(obj, attr_name, value)

    @property
    def wavelength_step_type(self):
        self._assert_all_wavelength_same("wavelength_step_type")
        val = self._all_states.wavelength.wavelength_step_type
        return RangeStepType.LIN if val is RangeStepType.NOT_SET or not val else val

    @wavelength_step_type.setter
    def wavelength_step_type(self, value):
        self._set_on_all_wavelength("wavelength_step_type", value)

    @property
    def wavelength_min(self):
        self._assert_all_wavelength_same("wavelength_low")
        val = self._all_states.wavelength.wavelength_low
        val = val[0] if isinstance(val, list) else val
        return self._get_val_or_default(val)

    @wavelength_min.setter
    def wavelength_min(self, value):
        value = [value] if not isinstance(value, list) else value
        self._set_on_all_wavelength("wavelength_low", value)

    @property
    def wavelength_max(self):
        self._assert_all_wavelength_same("wavelength_high")
        val = self._all_states.wavelength.wavelength_high
        val = val[0] if isinstance(val, list) else val
        return self._get_val_or_default(val)

    @wavelength_max.setter
    def wavelength_max(self, value):
        value = [value] if not isinstance(value, list) else value
        self._set_on_all_wavelength("wavelength_high", value)

    @property
    def wavelength_step(self):
        self._assert_all_wavelength_same("wavelength_step")
        val = self._all_states.wavelength.wavelength_step
        return self._get_val_or_default(val)

    @wavelength_step.setter
    def wavelength_step(self, value):
        self._set_on_all_wavelength("wavelength_step", value)

    @property
    def wavelength_range(self):
        val = self._wavelength_range
        return self._get_val_or_default(val)

    @wavelength_range.setter
    def wavelength_range(self, value):
        wav_start, wav_stop = parse_range_wavelength(value)
        self.wavelength_min = wav_start
        self.wavelength_max = wav_stop
        self._wavelength_range = value

    # ------------------------------------------------------------------------------------------------------------------
    # Scale properties
    # While the absolute scale can be set in the
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def absolute_scale(self):
        val = self._all_states.scale.scale
        return self._get_val_or_default(val)

    @absolute_scale.setter
    def absolute_scale(self, value):
        self._all_states.scale.scale = value

    @property
    def sample_height(self):
        val = self._all_states.scale.height
        return self._get_val_or_default(val)

    @sample_height.setter
    def sample_height(self, value):
        self._all_states.scale.height = value

    @property
    def sample_width(self):
        val = self._all_states.scale.width
        return self._get_val_or_default(val)

    @sample_width.setter
    def sample_width(self, value):
        self._all_states.scale.width = value

    @property
    def sample_thickness(self):
        val = self._all_states.scale.thickness
        return self._get_val_or_default(val)

    @sample_thickness.setter
    def sample_thickness(self, value):
        self._all_states.scale.thickness = value

    @property
    def sample_shape(self):
        val = self._all_states.scale.shape
        return val

    @sample_shape.setter
    def sample_shape(self, value):
        # We only set the value if it is not None. Note that it can be None if the sample shape selection
        #  is "Read from file"
        if value is not None:
            self._all_states.scale.shape = value

    @property
    def z_offset(self):
        val = self._all_states.move.sample_offset
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @z_offset.setter
    def z_offset(self, value):
        self._all_states.move.sample_offset = millimeter_2_meter(value)

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q Limits
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def q_1d_rebin_string(self):
        val = self._all_states.convert_to_q.q_1d_rebin_string
        return self._get_val_or_default(val)

    @q_1d_rebin_string.setter
    def q_1d_rebin_string(self, value):
        self._all_states.convert_to_q.q_1d_rebin_string = value

    @property
    def q_xy_max(self):
        val = self._all_states.convert_to_q.q_xy_max
        return self._get_val_or_default(val)

    @q_xy_max.setter
    def q_xy_max(self, value):
        self._all_states.convert_to_q.q_xy_max = value

    @property
    def q_xy_step(self):
        val = self._all_states.convert_to_q.q_xy_step
        return self._get_val_or_default(val)

    @q_xy_step.setter
    def q_xy_step(self, value):
        self._all_states.convert_to_q.q_xy_step = value

    @property
    def q_xy_step_type(self):
        # Can return None
        return self._all_states.convert_to_q.q_xy_step_type

    @q_xy_step_type.setter
    def q_xy_step_type(self, value):
        self._all_states.convert_to_q.q_xy_step_type = value

    @property
    def r_cut(self):
        val = self._all_states.convert_to_q.radius_cutoff
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @r_cut.setter
    def r_cut(self, value):
        self._all_states.convert_to_q.radius_cutoff = millimeter_2_meter(value)

    @property
    def w_cut(self):
        val = self._all_states.convert_to_q.wavelength_cutoff
        return self._get_val_or_default(val)

    @w_cut.setter
    def w_cut(self, value):
        self._all_states.convert_to_q.wavelength_cutoff = value

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def gravity_on_off(self):
        val = self._all_states.convert_to_q.use_gravity
        return self._get_val_or_default(val, True)

    @gravity_on_off.setter
    def gravity_on_off(self, value):
        self._all_states.convert_to_q.use_gravity = value

    @property
    def gravity_extra_length(self):
        val = self._all_states.convert_to_q.gravity_extra_length
        return self._get_val_or_default(val)

    @gravity_extra_length.setter
    def gravity_extra_length(self, value):
        self._all_states.convert_to_q.gravity_extra_length = value

    # ------------------------------------------------------------------------------------------------------------------
    # QResolution
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def use_q_resolution(self):
        val = self._all_states.convert_to_q.use_q_resolution
        return self._get_val_or_default(val, False)

    @use_q_resolution.setter
    def use_q_resolution(self, value):
        self._all_states.convert_to_q.use_q_resolution = value

    @property
    def q_resolution_source_a(self):
        val = self._all_states.convert_to_q.q_resolution_a1
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_source_a.setter
    def q_resolution_source_a(self, value):
        self._all_states.convert_to_q.q_resolution_a1 = millimeter_2_meter(value)

    @property
    def q_resolution_sample_a(self):
        val = self._all_states.convert_to_q.q_resolution_a2
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_sample_a.setter
    def q_resolution_sample_a(self, value):
        self._all_states.convert_to_q.q_resolution_a2 = millimeter_2_meter(value)

    @property
    def q_resolution_source_h(self):
        val = self._all_states.convert_to_q.q_resolution_h1
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_source_h.setter
    def q_resolution_source_h(self, value):
        self._all_states.convert_to_q.q_resolution_h1 = millimeter_2_meter(value)

    @property
    def q_resolution_sample_h(self):
        val = self._all_states.convert_to_q.q_resolution_h2
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_sample_h.setter
    def q_resolution_sample_h(self, value):
        self._all_states.convert_to_q.q_resolution_h2 = millimeter_2_meter(value)

    @property
    def q_resolution_source_w(self):
        val = self._all_states.convert_to_q.q_resolution_w1
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_source_w.setter
    def q_resolution_source_w(self, value):
        self._all_states.convert_to_q.q_resolution_w1 = millimeter_2_meter(value)

    @property
    def q_resolution_sample_w(self):
        val = self._all_states.convert_to_q.q_resolution_w2
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_sample_w.setter
    def q_resolution_sample_w(self, value):
        self._all_states.convert_to_q.q_resolution_w2 = millimeter_2_meter(value)

    @property
    def q_resolution_delta_r(self):
        val = self._all_states.convert_to_q.q_resolution_delta_r
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @q_resolution_delta_r.setter
    def q_resolution_delta_r(self, value):
        self._all_states.convert_to_q.q_resolution_delta_r = millimeter_2_meter(value)

    @property
    def q_resolution_moderator_file(self):
        val = self._all_states.convert_to_q.moderator_file
        return self._get_val_or_default(val)

    @q_resolution_moderator_file.setter
    def q_resolution_moderator_file(self, value):
        self._all_states.convert_to_q.moderator_file = value

    @property
    def q_resolution_collimation_length(self):
        val = self._all_states.convert_to_q.q_resolution_collimation_length
        return self._get_val_or_default(val)

    @q_resolution_collimation_length.setter
    def q_resolution_collimation_length(self, value):
        self._all_states.convert_to_q.q_resolution_collimation_length = value

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi limit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def phi_limit_min(self):
        val = self._all_states.mask.phi_min
        return self._get_val_or_default(val, "-90")

    @phi_limit_min.setter
    def phi_limit_min(self, value):
        self._all_states.mask.phi_min = value

    @property
    def phi_limit_max(self):
        val = self._all_states.mask.phi_max
        return self._get_val_or_default(val, "90")

    @phi_limit_max.setter
    def phi_limit_max(self, value):
        self._all_states.mask.phi_max = value

    @property
    def phi_limit_use_mirror(self):
        val = self._all_states.mask.use_mask_phi_mirror
        return self._get_val_or_default(val, True)

    @phi_limit_use_mirror.setter
    def phi_limit_use_mirror(self, value):
        self._all_states.mask.use_mask_phi_mirror = value

    # ------------------------------------------------------------------------------------------------------------------
    # Radius limit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def radius_limit_min(self):
        val = self._all_states.mask.radius_min
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @radius_limit_min.setter
    def radius_limit_min(self, value):
        self._all_states.mask.radius_min = millimeter_2_meter(value)

    @property
    def radius_limit_max(self):
        val = self._all_states.mask.radius_max
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @radius_limit_max.setter
    def radius_limit_max(self, value):
        self._all_states.mask.radius_max = millimeter_2_meter(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def mask_files(self):
        val = self._all_states.mask.mask_files
        return self._get_val_or_default(val, [])

    @mask_files.setter
    def mask_files(self, value):
        self._all_states.mask.mask_files = value

    # ------------------------------------------------------------------------------------------------------------------
    # Output name
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def output_name(self):
        val = self._all_states.save.user_specified_output_name
        return self._get_val_or_default(val)

    @output_name.setter
    def output_name(self, value):
        self._all_states.save.user_specified_output_name = value
