from sans.user_file.user_file_common import (OtherId, DetectorId, LimitsId, SetId, SampleId, MonId, TransId,
                                             event_binning_string_values, set_scales_entry, monitor_spectrum,
                                             simple_range, monitor_file, det_fit_range)
from sans.common.enums import (ReductionDimensionality, ISISReductionMode, RangeStepType, SampleShape, SaveType,
                               DetectorType)


class StateGuiModel(object):
    def __init__(self, user_file_items):
        super(StateGuiModel, self).__init__()
        self._user_file_items = user_file_items

    @property
    def settings(self):
        return self._user_file_items

    def get_simple_element(self, element_id, default_value):
        return self.get_simple_element_with_attribute(element_id, default_value)

    def set_simple_element(self, element_id, value):
        if element_id in self._user_file_items:
            del self._user_file_items[element_id]
        new_state_entries = {element_id: [value]}
        self._user_file_items.update(new_state_entries)

    def get_simple_element_with_attribute(self, element_id, default_value, attribute=None):
        if element_id in self._user_file_items:
            element = self._user_file_items[element_id][-1]
            return getattr(element, attribute) if attribute else element
        else:
            return default_value

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
        return self.get_simple_element(element_id=OtherId.use_compatibility_mode, default_value=False)

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.set_simple_element(element_id=OtherId.use_compatibility_mode, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Save Options
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def zero_error_free(self):
        if OtherId.save_as_zero_error_free in self._user_file_items:
            return self._user_file_items[OtherId.save_as_zero_error_free][-1]
        else:
            # Turn on zero error free saving by default
            return True

    @zero_error_free.setter
    def zero_error_free(self, value):
        if value is None:
            return
        if OtherId.save_as_zero_error_free in self._user_file_items:
            del self._user_file_items[OtherId.save_as_zero_error_free]
        new_state_entries = {OtherId.save_as_zero_error_free: [value]}
        self._user_file_items.update(new_state_entries)

    @property
    def save_types(self):
        return self.get_simple_element(element_id=OtherId.save_types, default_value=[SaveType.NXcanSAS])

    @save_types.setter
    def save_types(self, value):
        self.set_simple_element(element_id=OtherId.save_types, value=value)

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
        return self.get_simple_element_with_attribute(element_id=OtherId.event_slices,
                                                      default_value="",
                                                      attribute="value")

    @event_slices.setter
    def event_slices(self, value):
        if not value:
            return
        if OtherId.event_slices in self._user_file_items:
            del self._user_file_items[OtherId.event_slices]
        new_state_entries = {OtherId.event_slices: [event_binning_string_values(value=value)]}
        self._user_file_items.update(new_state_entries)

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction dimensionality
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_dimensionality(self):
        return self.get_simple_element_with_attribute(element_id=OtherId.reduction_dimensionality,
                                                      default_value=ReductionDimensionality.OneDim)

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value):
        if value is ReductionDimensionality.OneDim or value is ReductionDimensionality.TwoDim:
            if OtherId.reduction_dimensionality in self._user_file_items:
                del self._user_file_items[OtherId.reduction_dimensionality]
            new_state_entries = {OtherId.reduction_dimensionality: [value]}
            self._user_file_items.update(new_state_entries)
        else:
            raise ValueError("A reduction dimensionality was expected, got instead {}".format(value))

    # ------------------------------------------------------------------------------------------------------------------
    # Reduction Mode
    # ------------------------------------------------------------------------------------------------------------------
    def _remove_merged_fit(self, element_id):
        if element_id in self._user_file_items:
            del self._user_file_items[element_id]

    def _update_merged_fit(self, element_id, use_fit=None, use_q_range=None, q_start=None, q_stop=None):
        # If the setting is not in there, then add, else all is good
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]
        else:
            settings = [det_fit_range(start=None, stop=None, use_fit=False)]

        new_settings = []
        for setting in settings:
            new_use_fit = use_fit if use_fit is not None else setting.use_fit
            new_q_start = q_start if q_start is not None else settings.start
            new_q_stop = q_stop if q_stop is not None else settings.stop

            # If we don't want to use a custom q range, then we need to set the start and stop values to None
            if use_q_range is not None and use_q_range is False:
                new_q_start = None
                new_q_stop = None

            new_settings.append(det_fit_range(start=new_q_start, stop=new_q_stop, use_fit=new_use_fit))
        self._user_file_items.update({element_id: new_settings})

    @property
    def reduction_mode(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.reduction_mode,
                                                      default_value=ISISReductionMode.LAB)

    @reduction_mode.setter
    def reduction_mode(self, value):
        if (value is ISISReductionMode.LAB or value is ISISReductionMode.HAB or
            value is ISISReductionMode.Merged or value is ISISReductionMode.All):  # noqa
            if DetectorId.reduction_mode in self._user_file_items:
                del self._user_file_items[DetectorId.reduction_mode]
            new_state_entries = {DetectorId.reduction_mode: [value]}
            self._user_file_items.update(new_state_entries)
        else:
            raise ValueError("A reduction mode was expected, got instead {}".format(value))

    @property
    def merge_scale(self):
        return self.get_simple_element(element_id=DetectorId.rescale, default_value="")

    @merge_scale.setter
    def merge_scale(self, value):
        self.set_simple_element(element_id=DetectorId.rescale, value=value)

    @property
    def merge_shift(self):
        return self.get_simple_element(element_id=DetectorId.shift, default_value="")

    @merge_shift.setter
    def merge_shift(self, value):
        self.set_simple_element(element_id=DetectorId.shift, value=value)

    @property
    def merge_scale_fit(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.rescale_fit,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_scale_fit.setter
    def merge_scale_fit(self, value):
        self._update_merged_fit(element_id=DetectorId.rescale_fit, use_fit=value)

    @property
    def merge_shift_fit(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.shift_fit,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_shift_fit.setter
    def merge_shift_fit(self, value):
        self._update_merged_fit(element_id=DetectorId.shift_fit, use_fit=value)

    @property
    def merge_use_q_range(self):
        # There can be a q range setting for
        pass

    @merge_use_q_range.setter
    def merge_use_q_range(self, value):
        pass

    @property
    def merge_q_range_start(self):
        pass

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.shift_fit, q_start=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.rescale_fit, q_start=value)

    @property
    def merge_q_range_stop(self):
        pass

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.shift_fit, q_stop=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.rescale_fit, q_stop=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength properties
    # Note that the wavelength settings are being used in four sub-states:
    # - wavelength
    # - calculate_transmission
    # - normalize_to_monitor
    # - wavelength_and_pixel_adjustment
    # This is not something that needs to be known at this point, but it is good to know.
    # ------------------------------------------------------------------------------------------------------------------
    def _update_wavelength(self, min_value=None, max_value=None, step=None, step_type=None):
        if LimitsId.wavelength in self._user_file_items:
            settings = self._user_file_items[LimitsId.wavelength]
        else:
            # If the entry does not already exist, then add it. The -1. is an illegal input which should get overriden
            # and if not we want it to fail.
            settings = [simple_range(start=-1., stop=-1., step=-1., step_type=RangeStepType.Lin)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value else setting.start
            new_max = max_value if max_value else setting.stop
            new_step = step if step else setting.step
            new_step_type = step_type if step_type else setting.step_type
            new_setting = simple_range(start=new_min, stop=new_max, step=new_step, step_type=new_step_type)
            new_settings.append(new_setting)
        self._user_file_items.update({LimitsId.wavelength: new_settings})

    @property
    def wavelength_step_type(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.wavelength, default_value=RangeStepType.Lin,
                                                      attribute="step_type")

    @wavelength_step_type.setter
    def wavelength_step_type(self, value):
        self._update_wavelength(step_type=value)

    @property
    def wavelength_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.wavelength,
                                                      default_value="",
                                                      attribute="start")

    @wavelength_min.setter
    def wavelength_min(self, value):
        self._update_wavelength(min_value=value)

    @property
    def wavelength_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.wavelength,
                                                      default_value="",
                                                      attribute="stop")

    @wavelength_max.setter
    def wavelength_max(self, value):
        self._update_wavelength(max_value=value)

    @property
    def wavelength_step(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.wavelength,
                                                      default_value="",
                                                      attribute="step")

    @wavelength_step.setter
    def wavelength_step(self, value):
        self._update_wavelength(step=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale properties
    # While the absolute scale can be set in the
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def absolute_scale(self):
        return self.get_simple_element_with_attribute(element_id=SetId.scales,
                                                      default_value="",
                                                      attribute="s")

    @absolute_scale.setter
    def absolute_scale(self, value):
        if SetId.scales in self._user_file_items:
            settings = self._user_file_items[SetId.scales]
        else:
            settings = [set_scales_entry(s=100., a=0., b=0., c=0., d=0.)]

        new_settings = []
        for setting in settings:
            s_parameter = value if value else setting.s
            new_settings.append(set_scales_entry(s=s_parameter, a=0., b=0., c=0., d=0.))
        self._user_file_items.update({SetId.scales: new_settings})

    @property
    def sample_height(self):
        return self.get_simple_element(element_id=OtherId.sample_height, default_value="")

    @sample_height.setter
    def sample_height(self, value):
        self.set_simple_element(element_id=OtherId.sample_height, value=value)

    @property
    def sample_width(self):
        return self.get_simple_element(element_id=OtherId.sample_width, default_value="")

    @sample_width.setter
    def sample_width(self, value):
        self.set_simple_element(element_id=OtherId.sample_width, value=value)

    @property
    def sample_thickness(self):
        return self.get_simple_element(element_id=OtherId.sample_thickness, default_value="")

    @sample_thickness.setter
    def sample_thickness(self, value):
        self.set_simple_element(element_id=OtherId.sample_thickness, value=value)

    @property
    def sample_shape(self):
        return self.get_simple_element(element_id=OtherId.sample_shape, default_value=None)

    @sample_shape.setter
    def sample_shape(self, value):
        # We only set the value if it is not None. Note that it can be None if the sample shape selection
        #  is "Read from file"
        if value is not None:
            self.set_simple_element(element_id=OtherId.sample_shape, value=value)

    @property
    def z_offset(self):
        return self.get_simple_element(element_id=SampleId.offset, default_value="")

    @z_offset.setter
    def z_offset(self, value):
        self.set_simple_element(element_id=SampleId.offset, value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # ADJUSTMENT TAB
    # ==================================================================================================================
    # ==================================================================================================================
    def _update_incident_spectrum_info(self, spectrum=None, interpolate=None, is_trans=False):
        if MonId.spectrum in self._user_file_items:
            settings = self._user_file_items[MonId.spectrum]
        else:
            # If the entry does not already exist, then add it. The -1. is an illegal input which should get overridden
            # and if not we want it to fail.
            settings = [monitor_spectrum(spectrum=-1, is_trans=is_trans, interpolate=is_trans)]

        new_settings = []
        for setting in settings:
            # Only modify the settings which match the is_trans, selection. Else we muddle up the normalize to monitor
            # settings with the transmission settings.
            if setting.is_trans == is_trans:
                new_spectrum = spectrum if spectrum else setting.spectrum
                new_interpolate = interpolate if interpolate else setting.interpolate
                new_setting = monitor_spectrum(spectrum=new_spectrum, is_trans=is_trans,
                                               interpolate=new_interpolate)
                new_settings.append(new_setting)
            else:
                new_settings.append(setting)
        self._user_file_items.update({MonId.spectrum: new_settings})

    def _get_incident_spectrum_info(self, default_value, attribute, is_trans):
        if MonId.spectrum in self._user_file_items:
            settings = self._user_file_items[MonId.spectrum]
            if is_trans:
                settings = [setting for setting in settings if setting.is_trans]
            else:
                settings = [setting for setting in settings if not setting.is_trans]
            element = settings[-1]
            return getattr(element, attribute)
        else:
            return default_value

    # ------------------------------------------------------------------------------------------------------------------
    # Monitor normalization
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def normalization_incident_monitor(self):
        return self._get_incident_spectrum_info(default_value="", attribute="spectrum", is_trans=False)

    @normalization_incident_monitor.setter
    def normalization_incident_monitor(self, value):
        self._update_incident_spectrum_info(spectrum=value, is_trans=False)

    @property
    def normalization_interpolate(self):
        return self._get_incident_spectrum_info(default_value=False, attribute="interpolate", is_trans=False)

    @normalization_interpolate.setter
    def normalization_interpolate(self, value):
        self._update_incident_spectrum_info(interpolate=value, is_trans=False)

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def transmission_incident_monitor(self):
        return self._get_incident_spectrum_info(default_value="", attribute="spectrum", is_trans=True)

    @transmission_incident_monitor.setter
    def transmission_incident_monitor(self, value):
        self._update_incident_spectrum_info(spectrum=value, is_trans=True)

    @property
    def transmission_interpolate(self):
        return self._get_incident_spectrum_info(default_value=False, attribute="interpolate", is_trans=True)

    @transmission_interpolate.setter
    def transmission_interpolate(self, value):
        self._update_incident_spectrum_info(interpolate=value, is_trans=True)

    @property
    def transmission_roi_files(self):
        return self.get_simple_element(element_id=TransId.roi, default_value="")

    @transmission_roi_files.setter
    def transmission_roi_files(self, value):
        self.set_simple_element(element_id=TransId.roi, value=value)

    @property
    def transmission_mask_files(self):
        return self.get_simple_element(element_id=TransId.mask, default_value="")

    @transmission_mask_files.setter
    def transmission_mask_files(self, value):
        self.set_simple_element(element_id=TransId.mask, value=value)

    @property
    def transmission_radius(self):
        return self.get_simple_element(element_id=TransId.radius, default_value="")

    @transmission_radius.setter
    def transmission_radius(self, value):
        self.set_simple_element(element_id=TransId.radius, value=value)

    @property
    def transmission_monitor(self):
        return self.get_simple_element(element_id=TransId.spec, default_value=3)

    @transmission_monitor.setter
    def transmission_monitor(self, value):
        self.set_simple_element(element_id=TransId.spec, value=value)

    @property
    def transmission_m4_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        return self.get_simple_element(element_id=TransId.spec_shift, default_value="")

    @transmission_m4_shift.setter
    def transmission_m4_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        self.set_simple_element(element_id=TransId.spec_shift, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength- and pixel-adjustment files
    # ------------------------------------------------------------------------------------------------------------------
    def _get_adjustment_file_setting(self, element_id, detector_type, default_value):
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]

            # Separate out the correct detector type
            settings = [setting for setting in settings if setting.detector_type is detector_type]
            if settings:
                setting = settings[-1]
                return setting.file_path
        return default_value

    def _set_adjustment_file_setting(self, element_id, detector_type, file_path):
        # Check if we already have items for the particular detector type
        settings_with_correct_detector = []
        settings = []
        if element_id in self._user_file_items:
            settings = self._user_file_items[element_id]
            settings_with_correct_detector = [setting for setting in settings if setting.detector_type is detector_type]

        if not (settings and settings_with_correct_detector):
            settings.append(monitor_file(file_path="", detector_type=detector_type))

        # At this point we have settings with the desired detector type
        new_settings = []
        for setting in settings:
            if setting.detector_type is detector_type:
                new_settings.append(monitor_file(file_path=file_path, detector_type=setting.detector_type))
            else:
                new_settings.append(setting)
        self._user_file_items.update({element_id: new_settings})

    @property
    def pixel_adjustment_det_1(self):
        return self._get_adjustment_file_setting(element_id=MonId.flat, detector_type=DetectorType.LAB,
                                                 default_value="")

    @pixel_adjustment_det_1.setter
    def pixel_adjustment_det_1(self, value):
        self._set_adjustment_file_setting(element_id=MonId.flat, detector_type=DetectorType.LAB, file_path=value)

    @property
    def pixel_adjustment_det_2(self):
        return self._get_adjustment_file_setting(element_id=MonId.flat, detector_type=DetectorType.HAB,
                                                 default_value="")

    @pixel_adjustment_det_2.setter
    def pixel_adjustment_det_2(self, value):
        self._set_adjustment_file_setting(element_id=MonId.flat, detector_type=DetectorType.HAB, file_path=value)

    @property
    def wavelength_adjustment_det_1(self):
        return self._get_adjustment_file_setting(element_id=MonId.direct, detector_type=DetectorType.LAB,
                                                 default_value="")

    @wavelength_adjustment_det_1.setter
    def wavelength_adjustment_det_1(self, value):
        self._set_adjustment_file_setting(element_id=MonId.direct, detector_type=DetectorType.LAB, file_path=value)

    @property
    def wavelength_adjustment_det_2(self):
        return self._get_adjustment_file_setting(element_id=MonId.direct, detector_type=DetectorType.HAB,
                                                 default_value="")

    @wavelength_adjustment_det_2.setter
    def wavelength_adjustment_det_2(self, value):
        self._set_adjustment_file_setting(element_id=MonId.direct, detector_type=DetectorType.HAB, file_path=value)
