# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The state gui model contains all the reduction information which is not explicitly available in the data table.

This is one of the two models which is used for the data reduction. It contains generally all the settings which
are not available in the model associated with the data table.
"""

from __future__ import (absolute_import, division, print_function)

from mantid.py3compat import ensure_str
from sans.common.enums import (ReductionDimensionality, ReductionMode, RangeStepType, SaveType,
                               DetectorType)
from sans.gui_logic.models.model_common import ModelCommon
from sans.user_file.settings_tags import (OtherId, DetectorId, LimitsId, SetId, SampleId, GravityId,
                                          QResolutionId, MaskId, event_binning_string_values, set_scales_entry,
                                          simple_range, det_fit_range,
                                          q_rebin_values, mask_angle_entry, range_entry, position_entry)


class StateGuiModel(ModelCommon):
    def __init__(self, user_file_items):
        super(StateGuiModel, self).__init__(user_file_items)
        self._user_file_items = user_file_items

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    @property
    def settings(self):
        return self._user_file_items

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
        return self.get_simple_element(element_id=OtherId.USE_COMPATIBILITY_MODE, default_value=True)

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.set_simple_element(element_id=OtherId.USE_COMPATIBILITY_MODE, value=value)

    @property
    def event_slice_optimisation(self):
        return self.get_simple_element(element_id=OtherId.USE_EVENT_SLICE_OPTIMISATION, default_value=False)

    @event_slice_optimisation.setter
    def event_slice_optimisation(self, value):
        self.set_simple_element(element_id=OtherId.USE_EVENT_SLICE_OPTIMISATION, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Save Options
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def zero_error_free(self):
        if OtherId.SAVE_AS_ZERO_ERROR_FREE in self._user_file_items:
            return self._user_file_items[OtherId.SAVE_AS_ZERO_ERROR_FREE][-1]
        else:
            # Turn on zero error free saving by default
            return True

    @zero_error_free.setter
    def zero_error_free(self, value):
        if value is None:
            return
        if OtherId.SAVE_AS_ZERO_ERROR_FREE in self._user_file_items:
            del self._user_file_items[OtherId.SAVE_AS_ZERO_ERROR_FREE]
        new_state_entries = {OtherId.SAVE_AS_ZERO_ERROR_FREE: [value]}
        self._user_file_items.update(new_state_entries)

    @property
    def save_types(self):
        return self.get_simple_element(element_id=OtherId.SAVE_TYPES, default_value=[SaveType.NX_CAN_SAS])

    @save_types.setter
    def save_types(self, value):
        self.set_simple_element(element_id=OtherId.SAVE_TYPES, value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # BeamCentre TAB
    # ==================================================================================================================
    # ==================================================================================================================
    @property
    def lab_pos_1(self):
        return self.get_simple_element_with_attribute(element_id=SetId.CENTRE, default_value='', attribute="pos1")

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self._update_centre(pos_1=value)

    @property
    def lab_pos_2(self):
        return self.get_simple_element_with_attribute(element_id=SetId.CENTRE, default_value='', attribute="pos2")

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self._update_centre(pos_2=value)

    @property
    def hab_pos_1(self):
        return self.get_simple_element_with_attribute(element_id=SetId.CENTRE_HAB, default_value='', attribute="pos1")

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        self._update_centre(pos_1=value)

    @property
    def hab_pos_2(self):
        return self.get_simple_element_with_attribute(element_id=SetId.CENTRE_HAB, default_value='', attribute="pos2")

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        self._update_centre(pos_2=value)

    def _update_centre(self, pos_1=None, pos_2=None, detector_type=None):
        if SetId.CENTRE in self._user_file_items:
            settings = self._user_file_items[SetId.CENTRE]
        else:
            # If the entry does not already exist, then add it. The -1. is an illegal input which should get overridden
            # and if not we want it to fail.
            settings = [position_entry(pos1=0.0, pos2=0.0, detector_type=DetectorType.LAB)]

        new_settings = []
        for setting in settings:
            new_pos1 = pos_1 if pos_1 else setting.pos1
            new_pos2 = pos_2 if pos_2 else setting.pos2
            new_detector_type = detector_type if detector_type else setting.detector_type
            new_setting = position_entry(pos1=new_pos1, pos2=new_pos2, detector_type=new_detector_type)
            new_settings.append(new_setting)
        self._user_file_items.update({SetId.CENTRE: new_settings})
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
        return self.get_simple_element_with_attribute(element_id=OtherId.EVENT_SLICES,
                                                      default_value="",
                                                      attribute="value")

    @event_slices.setter
    def event_slices(self, value):
        if not value:
            return
        if OtherId.EVENT_SLICES in self._user_file_items:
            del self._user_file_items[OtherId.EVENT_SLICES]
        new_state_entries = {OtherId.EVENT_SLICES: [event_binning_string_values(value=value)]}
        self._user_file_items.update(new_state_entries)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_dimensionality(self):
        return self.get_simple_element_with_attribute(element_id=OtherId.REDUCTION_DIMENSIONALITY,
                                                      default_value=ReductionDimensionality.ONE_DIM)

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value):
        if value is ReductionDimensionality.ONE_DIM or value is ReductionDimensionality.TWO_DIM:
            if OtherId.REDUCTION_DIMENSIONALITY in self._user_file_items:
                del self._user_file_items[OtherId.REDUCTION_DIMENSIONALITY]
            new_state_entries = {OtherId.REDUCTION_DIMENSIONALITY: [value]}
            self._user_file_items.update(new_state_entries)
        else:
            raise ValueError("A reduction dimensionality was expected, got instead {}".format(value))

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction Mode
    # ------------------------------------------------------------------------------------------------------------------
    def _update_merged_fit(self, element_id, use_fit=None, use_q_range=None, q_start=None, q_stop=None):
        # If the setting is not in there, then add, else all is good
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]
        else:
            settings = [det_fit_range(start=None, stop=None, use_fit=False)]

        new_settings = []
        for setting in settings:
            new_use_fit = use_fit if use_fit is not None else setting.use_fit
            new_q_start = q_start if q_start is not None else setting.start
            new_q_stop = q_stop if q_stop is not None else setting.stop

            # If we don't want to use a custom q range, then we need to set the start and stop values to None
            if use_q_range is not None and use_q_range is False:
                new_q_start = None
                new_q_stop = None

            new_settings.append(det_fit_range(start=new_q_start, stop=new_q_stop, use_fit=new_use_fit))
        self._user_file_items.update({element_id: new_settings})

    def get_merge_range(self, default_value):
        q_start = []
        q_stop = []

        settings = []
        if DetectorId.RESCALE_FIT in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.RESCALE_FIT])
        if DetectorId.SHIFT_FIT in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.SHIFT_FIT])

        for setting in settings:
            if setting.start is not None:
                q_start.append(setting.start)

            if setting.stop is not None:
                q_stop.append(setting.stop)

        q_range_start = min(q_start) if q_start else default_value
        q_range_stop = max(q_stop) if q_stop else default_value
        return q_range_start, q_range_stop

    def get_merge_overlap(self, default_value):
        q_start = []
        q_stop = []

        settings = []
        if DetectorId.MERGE_RANGE in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.MERGE_RANGE])

        for setting in settings:
            if setting.start is not None:
                q_start.append(setting.start)

            if setting.stop is not None:
                q_stop.append(setting.stop)

        q_range_start = min(q_start) if q_start else default_value
        q_range_stop = max(q_stop) if q_stop else default_value
        return q_range_start, q_range_stop

    @property
    def reduction_mode(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.REDUCTION_MODE,
                                                      default_value=ReductionMode.LAB)

    @reduction_mode.setter
    def reduction_mode(self, value):
        if (value is ReductionMode.LAB or value is ReductionMode.HAB or
            value is ReductionMode.MERGED or value is ReductionMode.ALL):  # noqa
            if DetectorId.REDUCTION_MODE in self._user_file_items:
                del self._user_file_items[DetectorId.REDUCTION_MODE]
            new_state_entries = {DetectorId.REDUCTION_MODE: [value]}
            self._user_file_items.update(new_state_entries)
        else:
            raise ValueError("A reduction mode was expected, got instead {}".format(value))

    @property
    def merge_scale(self):
        return self.get_simple_element(element_id=DetectorId.RESCALE, default_value="1.0")

    @merge_scale.setter
    def merge_scale(self, value):
        self.set_simple_element(element_id=DetectorId.RESCALE, value=value)

    @property
    def merge_shift(self):
        return self.get_simple_element(element_id=DetectorId.SHIFT, default_value="0.0")

    @merge_shift.setter
    def merge_shift(self, value):
        self.set_simple_element(element_id=DetectorId.SHIFT, value=value)

    @property
    def merge_scale_fit(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.RESCALE_FIT,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_scale_fit.setter
    def merge_scale_fit(self, value):
        self._update_merged_fit(element_id=DetectorId.RESCALE_FIT, use_fit=value)

    @property
    def merge_shift_fit(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.SHIFT_FIT,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_shift_fit.setter
    def merge_shift_fit(self, value):
        self._update_merged_fit(element_id=DetectorId.SHIFT_FIT, use_fit=value)

    @property
    def merge_q_range_start(self):
        q_range_start, _ = self.get_merge_range(default_value="")
        return q_range_start

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.SHIFT_FIT, q_start=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.RESCALE_FIT, q_start=value)

    @property
    def merge_q_range_stop(self):
        _, q_range_stop = self.get_merge_range(default_value="")
        return q_range_stop

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.SHIFT_FIT, q_stop=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.RESCALE_FIT, q_stop=value)

    @property
    def merge_mask(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.MERGE_RANGE,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_mask.setter
    def merge_mask(self, value):
        self._update_merged_fit(element_id=DetectorId.MERGE_RANGE, use_fit=value)

    @property
    def merge_max(self):
        _, q_range_stop = self.get_merge_overlap(default_value=None)
        return q_range_stop

    @merge_max.setter
    def merge_max(self, value):
        self._update_merged_fit(element_id=DetectorId.MERGE_RANGE, q_stop=value)

    @property
    def merge_min(self):
        q_range_start, _ = self.get_merge_overlap(default_value=None)
        return q_range_start

    @merge_min.setter
    def merge_min(self, value):
        self._update_merged_fit(element_id=DetectorId.MERGE_RANGE, q_start=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_binning(self):
        return self.get_simple_element(element_id=LimitsId.EVENTS_BINNING, default_value="")

    @event_binning.setter
    def event_binning(self, value):
        self.set_simple_element(element_id=LimitsId.EVENTS_BINNING, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength properties
    # Note that the wavelength settings are being used in four sub-states:
    # - wavelength
    # - calculate_transmission
    # - normalize_to_monitor
    # - wavelength_and_pixel_adjustment
    # This is not something that needs to be known at this point, but it is good to know.
    # ------------------------------------------------------------------------------------------------------------------
    def _update_wavelength(self, min_value=None, max_value=None, step=None, step_type=None, wavelength_range=None):
        if LimitsId.WAVELENGTH in self._user_file_items:
            settings = self._user_file_items[LimitsId.WAVELENGTH]
        else:
            # If the entry does not already exist, then add it. The -1. is an illegal input which should get overridden
            # and if not we want it to fail.
            settings = [simple_range(start=-1., stop=-1., step=-1., step_type=RangeStepType.LIN)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value else setting.start
            new_max = max_value if max_value else setting.stop
            new_step = step if step else setting.step
            new_step_type = step_type if step_type else setting.step_type
            new_setting = simple_range(start=new_min, stop=new_max, step=new_step, step_type=new_step_type)
            new_settings.append(new_setting)
        self._user_file_items.update({LimitsId.WAVELENGTH: new_settings})

        if wavelength_range:
            if OtherId.WAVELENGTH_RANGE in self._user_file_items:
                settings = self._user_file_items[OtherId.WAVELENGTH_RANGE]
            else:
                settings = [""]

            new_settings = []
            for setting in settings:
                new_range = wavelength_range if wavelength_range else setting
                new_settings.append(new_range)
            self._user_file_items.update({OtherId.WAVELENGTH_RANGE: new_settings})

    @property
    def wavelength_step_type(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.WAVELENGTH, default_value=RangeStepType.LIN,
                                                      attribute="step_type")

    @wavelength_step_type.setter
    def wavelength_step_type(self, value):
        self._update_wavelength(step_type=value)

    @property
    def wavelength_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.WAVELENGTH,
                                                      default_value="",
                                                      attribute="start")

    @wavelength_min.setter
    def wavelength_min(self, value):
        self._update_wavelength(min_value=value)

    @property
    def wavelength_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.WAVELENGTH,
                                                      default_value="",
                                                      attribute="stop")

    @wavelength_max.setter
    def wavelength_max(self, value):
        self._update_wavelength(max_value=value)

    @property
    def wavelength_step(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.WAVELENGTH,
                                                      default_value="",
                                                      attribute="step")

    @wavelength_step.setter
    def wavelength_step(self, value):
        self._update_wavelength(step=value)

    @property
    def wavelength_range(self):
        return self.get_simple_element(element_id=OtherId.WAVELENGTH_RANGE, default_value="")

    @wavelength_range.setter
    def wavelength_range(self, value):
        self._update_wavelength(wavelength_range=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale properties
    # While the absolute scale can be set in the
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def absolute_scale(self):
        return self.get_simple_element_with_attribute(element_id=SetId.SCALES,
                                                      default_value="",
                                                      attribute="s")

    @absolute_scale.setter
    def absolute_scale(self, value):
        if SetId.SCALES in self._user_file_items:
            settings = self._user_file_items[SetId.SCALES]
        else:
            settings = [set_scales_entry(s=100., a=0., b=0., c=0., d=0.)]

        new_settings = []
        for setting in settings:
            s_parameter = value if value else setting.s
            new_settings.append(set_scales_entry(s=s_parameter, a=0., b=0., c=0., d=0.))
        self._user_file_items.update({SetId.SCALES: new_settings})

    @property
    def sample_height(self):
        return self.get_simple_element(element_id=OtherId.SAMPLE_HEIGHT, default_value="")

    @sample_height.setter
    def sample_height(self, value):
        self.set_simple_element(element_id=OtherId.SAMPLE_HEIGHT, value=value)

    @property
    def sample_width(self):
        return self.get_simple_element(element_id=OtherId.SAMPLE_WIDTH, default_value="")

    @sample_width.setter
    def sample_width(self, value):
        self.set_simple_element(element_id=OtherId.SAMPLE_WIDTH, value=value)

    @property
    def sample_thickness(self):
        return self.get_simple_element(element_id=OtherId.SAMPLE_THICKNESS, default_value="")

    @sample_thickness.setter
    def sample_thickness(self, value):
        self.set_simple_element(element_id=OtherId.SAMPLE_THICKNESS, value=value)

    @property
    def sample_shape(self):
        return self.get_simple_element(element_id=OtherId.SAMPLE_SHAPE, default_value=None)

    @sample_shape.setter
    def sample_shape(self, value):
        # We only set the value if it is not None. Note that it can be None if the sample shape selection
        #  is "Read from file"
        if value is not None:
            self.set_simple_element(element_id=OtherId.SAMPLE_SHAPE, value=value)

    @property
    def z_offset(self):
        return self.get_simple_element(element_id=SampleId.OFFSET, default_value="")

    @z_offset.setter
    def z_offset(self, value):
        self.set_simple_element(element_id=SampleId.OFFSET, value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q Limits
    # ------------------------------------------------------------------------------------------------------------------
    def _set_q_1d_limits(self, min_value=None, max_value=None, rebin_string=None):
        element_id = LimitsId.Q
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]
        else:
            settings = [q_rebin_values(min=None, max=None, rebin_string=None)]

        # At this point we have settings with the desired detector type
        new_settings = []
        for setting in settings:
            new_min = min_value if min_value is not None else setting.min
            new_max = max_value if max_value is not None else setting.max
            new_rebin_string = rebin_string if rebin_string is not None else setting.rebin_string
            new_settings.append(q_rebin_values(min=new_min, max=new_max, rebin_string=new_rebin_string))
        self._user_file_items.update({element_id: new_settings})

    def _set_q_xy_limits(self, stop_value=None, step_value=None, step_type_value=None):
        element_id = LimitsId.QXY
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]
        else:
            settings = [simple_range(start=None, stop=None, step=None, step_type=None)]

        # At this point we have settings with the desired detector type
        new_settings = []
        for setting in settings:
            new_stop = stop_value if stop_value is not None else setting.stop
            new_step = step_value if step_value is not None else setting.step
            new_step_type_value = step_type_value if step_type_value is not None else setting.step_type
            new_settings.append(simple_range(start=None, stop=new_stop, step=new_step, step_type=new_step_type_value))
        self._user_file_items.update({element_id: new_settings})

    @property
    def q_1d_rebin_string(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.Q, default_value="",
                                                      attribute="rebin_string")

    @q_1d_rebin_string.setter
    def q_1d_rebin_string(self, value):
        self._set_q_1d_limits(rebin_string=ensure_str(value))

    @property
    def q_xy_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.QXY, default_value="",
                                                      attribute="stop")

    @q_xy_max.setter
    def q_xy_max(self, value):
        self._set_q_xy_limits(stop_value=value)

    @property
    def q_xy_step(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.QXY, default_value="",
                                                      attribute="step")

    @q_xy_step.setter
    def q_xy_step(self, value):
        self._set_q_xy_limits(step_value=value)

    @property
    def q_xy_step_type(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.QXY, default_value=None,
                                                      attribute="step_type")

    @q_xy_step_type.setter
    def q_xy_step_type(self, value):
        self._set_q_xy_limits(step_type_value=value)

    @property
    def r_cut(self):
        return self.get_simple_element(element_id=LimitsId.RADIUS_CUT, default_value="")

    @r_cut.setter
    def r_cut(self, value):
        self.set_simple_element(element_id=LimitsId.RADIUS_CUT, value=value)

    @property
    def w_cut(self):
        return self.get_simple_element(element_id=LimitsId.WAVELENGTH_CUT, default_value="")

    @w_cut.setter
    def w_cut(self, value):
        self.set_simple_element(element_id=LimitsId.WAVELENGTH_CUT, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def gravity_on_off(self):
        return self.get_simple_element(element_id=GravityId.ON_OFF, default_value=True)

    @gravity_on_off.setter
    def gravity_on_off(self, value):
        self.set_simple_element(element_id=GravityId.ON_OFF, value=value)

    @property
    def gravity_extra_length(self):
        return self.get_simple_element(element_id=GravityId.EXTRA_LENGTH, default_value="")

    @gravity_extra_length.setter
    def gravity_extra_length(self, value):
        self.set_simple_element(element_id=GravityId.EXTRA_LENGTH, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # QResolution
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def use_q_resolution(self):
        return self.get_simple_element(element_id=QResolutionId.ON, default_value=False)

    @use_q_resolution.setter
    def use_q_resolution(self, value):
        self.set_simple_element(element_id=QResolutionId.ON, value=value)

    @property
    def q_resolution_source_a(self):
        return self.get_simple_element(element_id=QResolutionId.A1, default_value="")

    @q_resolution_source_a.setter
    def q_resolution_source_a(self, value):
        self.set_simple_element(element_id=QResolutionId.A1, value=value)

    @property
    def q_resolution_sample_a(self):
        return self.get_simple_element(element_id=QResolutionId.A2, default_value="")

    @q_resolution_sample_a.setter
    def q_resolution_sample_a(self, value):
        self.set_simple_element(element_id=QResolutionId.A2, value=value)

    @property
    def q_resolution_source_h(self):
        return self.get_simple_element(element_id=QResolutionId.H1, default_value="")

    @q_resolution_source_h.setter
    def q_resolution_source_h(self, value):
        self.set_simple_element(element_id=QResolutionId.H1, value=value)

    @property
    def q_resolution_sample_h(self):
        return self.get_simple_element(element_id=QResolutionId.H2, default_value="")

    @q_resolution_sample_h.setter
    def q_resolution_sample_h(self, value):
        self.set_simple_element(element_id=QResolutionId.H2, value=value)

    @property
    def q_resolution_source_w(self):
        return self.get_simple_element(element_id=QResolutionId.W1, default_value="")

    @q_resolution_source_w.setter
    def q_resolution_source_w(self, value):
        self.set_simple_element(element_id=QResolutionId.W1, value=value)

    @property
    def q_resolution_sample_w(self):
        return self.get_simple_element(element_id=QResolutionId.W2, default_value="")

    @q_resolution_sample_w.setter
    def q_resolution_sample_w(self, value):
        self.set_simple_element(element_id=QResolutionId.W2, value=value)

    @property
    def q_resolution_delta_r(self):
        return self.get_simple_element(element_id=QResolutionId.DELTA_R, default_value="")

    @q_resolution_delta_r.setter
    def q_resolution_delta_r(self, value):
        self.set_simple_element(element_id=QResolutionId.DELTA_R, value=value)

    @property
    def q_resolution_moderator_file(self):
        return self.get_simple_element(element_id=QResolutionId.MODERATOR, default_value="")

    @q_resolution_moderator_file.setter
    def q_resolution_moderator_file(self, value):
        self.set_simple_element(element_id=QResolutionId.MODERATOR, value=value)

    @property
    def q_resolution_collimation_length(self):
        return self.get_simple_element(element_id=QResolutionId.COLLIMATION_LENGTH, default_value="")

    @q_resolution_collimation_length.setter
    def q_resolution_collimation_length(self, value):
        self.set_simple_element(element_id=QResolutionId.COLLIMATION_LENGTH, value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi limit
    # ------------------------------------------------------------------------------------------------------------------
    def _set_phi_limit(self, min_value=None, max_value=None, use_mirror=None):
        if LimitsId.ANGLE in self._user_file_items:
            settings = self._user_file_items[LimitsId.ANGLE]
        else:
            settings = [mask_angle_entry(min=None, max=None, use_mirror=False)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value is not None else setting.min
            new_max = max_value if max_value is not None else setting.max
            new_use_mirror = use_mirror if use_mirror is not None else setting.use_mirror
            new_settings.append(mask_angle_entry(min=new_min, max=new_max, use_mirror=new_use_mirror))
        self._user_file_items.update({LimitsId.ANGLE: new_settings})

    @property
    def phi_limit_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.ANGLE, attribute="min", default_value="-90")

    @phi_limit_min.setter
    def phi_limit_min(self, value):
        self._set_phi_limit(min_value=value)

    @property
    def phi_limit_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.ANGLE, attribute="max", default_value="90")

    @phi_limit_max.setter
    def phi_limit_max(self, value):
        self._set_phi_limit(max_value=value)

    @property
    def phi_limit_use_mirror(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.ANGLE, attribute="use_mirror", default_value=True)  # noqa

    @phi_limit_use_mirror.setter
    def phi_limit_use_mirror(self, value):
        self._set_phi_limit(use_mirror=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Radius limit
    # ------------------------------------------------------------------------------------------------------------------
    def _set_radius_limit(self, min_value=None, max_value=None):
        if LimitsId.RADIUS in self._user_file_items:
            settings = self._user_file_items[LimitsId.RADIUS]
        else:
            settings = [range_entry(start=None, stop=None)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value is not None else setting.start
            new_max = max_value if max_value is not None else setting.stop
            new_settings.append(range_entry(start=new_min, stop=new_max))
        self._user_file_items.update({LimitsId.RADIUS: new_settings})

    @property
    def radius_limit_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.RADIUS, attribute="start", default_value="")

    @radius_limit_min.setter
    def radius_limit_min(self, value):
        self._set_radius_limit(min_value=value)

    @property
    def radius_limit_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.RADIUS, attribute="stop", default_value="")

    @radius_limit_max.setter
    def radius_limit_max(self, value):
        self._set_radius_limit(max_value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def mask_files(self):
        if MaskId.FILE in self._user_file_items:
            return self._user_file_items[MaskId.FILE]
        return []

    @mask_files.setter
    def mask_files(self, value):
        if value is None:
            return
        if MaskId.FILE in self._user_file_items:
            del self._user_file_items[MaskId.FILE]
        new_state_entries = {MaskId.FILE: value}
        self._user_file_items.update(new_state_entries)

    # ------------------------------------------------------------------------------------------------------------------
    # Output name
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def output_name(self):
        return self.get_simple_element(element_id=OtherId.USER_SPECIFIED_OUTPUT_NAME, default_value="")

    @output_name.setter
    def output_name(self, value):
        self.set_simple_element(element_id=OtherId.USER_SPECIFIED_OUTPUT_NAME, value=value)
