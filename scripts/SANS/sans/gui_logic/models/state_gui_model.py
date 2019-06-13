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

from sans.user_file.settings_tags import (OtherId, DetectorId, LimitsId, SetId, SampleId, MonId, TransId, GravityId,
                                          QResolutionId, FitId, MaskId, event_binning_string_values, set_scales_entry,
                                          monitor_spectrum, simple_range, monitor_file, det_fit_range,
                                          q_rebin_values, fit_general, mask_angle_entry, range_entry, position_entry)
from sans.common.enums import (ReductionDimensionality, ISISReductionMode, RangeStepType, SaveType,
                               DetectorType, DataType, FitType, SANSInstrument)


class StateGuiModel(object):
    def __init__(self, user_file_items):
        super(StateGuiModel, self).__init__()
        self._user_file_items = user_file_items

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

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
    @property
    def instrument(self):
        return self.get_simple_element(element_id=DetectorId.instrument, default_value=SANSInstrument.NoInstrument)

    # ------------------------------------------------------------------------------------------------------------------
    # Compatibility Mode Options
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def compatibility_mode(self):
        return self.get_simple_element(element_id=OtherId.use_compatibility_mode, default_value=True)

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.set_simple_element(element_id=OtherId.use_compatibility_mode, value=value)

    @property
    def event_slice_optimisation(self):
        return self.get_simple_element(element_id=OtherId.use_event_slice_optimisation, default_value=False)

    @event_slice_optimisation.setter
    def event_slice_optimisation(self, value):
        self.set_simple_element(element_id=OtherId.use_event_slice_optimisation, value=value)

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
    # BeamCentre TAB
    # ==================================================================================================================
    # ==================================================================================================================
    @property
    def lab_pos_1(self):
        return self.get_simple_element_with_attribute(element_id=SetId.centre, default_value='', attribute="pos1")

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self._update_centre(pos_1=value)

    @property
    def lab_pos_2(self):
        return self.get_simple_element_with_attribute(element_id=SetId.centre, default_value='', attribute="pos2")

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self._update_centre(pos_2=value)

    @property
    def hab_pos_1(self):
        return self.get_simple_element_with_attribute(element_id=SetId.centre_HAB, default_value='', attribute="pos1")

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        self._update_centre(pos_1=value)

    @property
    def hab_pos_2(self):
        return self.get_simple_element_with_attribute(element_id=SetId.centre_HAB, default_value='', attribute="pos2")

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        self._update_centre(pos_2=value)

    def _update_centre(self, pos_1=None, pos_2=None, detector_type=None):
        if SetId.centre in self._user_file_items:
            settings = self._user_file_items[SetId.centre]
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
        self._user_file_items.update({SetId.centre: new_settings})
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
        if DetectorId.rescale_fit in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.rescale_fit])
        if DetectorId.shift_fit in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.shift_fit])

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
        if DetectorId.merge_range in self._user_file_items:
            settings.extend(self._user_file_items[DetectorId.merge_range])

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
        return self.get_simple_element(element_id=DetectorId.rescale, default_value="1.0")

    @merge_scale.setter
    def merge_scale(self, value):
        self.set_simple_element(element_id=DetectorId.rescale, value=value)

    @property
    def merge_shift(self):
        return self.get_simple_element(element_id=DetectorId.shift, default_value="0.0")

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
    def merge_q_range_start(self):
        q_range_start, _ = self.get_merge_range(default_value="")
        return q_range_start

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.shift_fit, q_start=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.rescale_fit, q_start=value)

    @property
    def merge_q_range_stop(self):
        _, q_range_stop = self.get_merge_range(default_value="")
        return q_range_stop

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        # Update for the shift
        self._update_merged_fit(element_id=DetectorId.shift_fit, q_stop=value)
        # Update for the scale
        self._update_merged_fit(element_id=DetectorId.rescale_fit, q_stop=value)

    @property
    def merge_mask(self):
        return self.get_simple_element_with_attribute(element_id=DetectorId.merge_range,
                                                      default_value=False,
                                                      attribute="use_fit")

    @merge_mask.setter
    def merge_mask(self, value):
        self._update_merged_fit(element_id=DetectorId.merge_range, use_fit=value)

    @property
    def merge_max(self):
        _, q_range_stop = self.get_merge_overlap(default_value=None)
        return q_range_stop

    @merge_max.setter
    def merge_max(self, value):
        self._update_merged_fit(element_id=DetectorId.merge_range, q_stop=value)

    @property
    def merge_min(self):
        q_range_start, _ = self.get_merge_overlap(default_value=None)
        return q_range_start

    @merge_min.setter
    def merge_min(self, value):
        self._update_merged_fit(element_id=DetectorId.merge_range, q_start=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Event binning for compatibility mode
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_binning(self):
        return self.get_simple_element(element_id=LimitsId.events_binning, default_value="")

    @event_binning.setter
    def event_binning(self, value):
        self.set_simple_element(element_id=LimitsId.events_binning, value=value)

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
        if LimitsId.wavelength in self._user_file_items:
            settings = self._user_file_items[LimitsId.wavelength]
        else:
            # If the entry does not already exist, then add it. The -1. is an illegal input which should get overridden
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

        if wavelength_range:
            if OtherId.wavelength_range in self._user_file_items:
                settings = self._user_file_items[OtherId.wavelength_range]
            else:
                settings = [""]

            new_settings = []
            for setting in settings:
                new_range = wavelength_range if wavelength_range else setting
                new_settings.append(new_range)
            self._user_file_items.update({OtherId.wavelength_range: new_settings})

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

    @property
    def wavelength_range(self):
        return self.get_simple_element(element_id=OtherId.wavelength_range, default_value="")

    @wavelength_range.setter
    def wavelength_range(self, value):
        self._update_wavelength(wavelength_range=value)

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
    def _update_incident_spectrum_info(self, spectrum=None, interpolate=False, is_trans=False):
        if MonId.spectrum in self._user_file_items:
            settings = self._user_file_items[MonId.spectrum]
        else:
            # If the entry does not already exist, then add it.
            settings = [monitor_spectrum(spectrum=spectrum, is_trans=is_trans, interpolate=interpolate)]

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
    def transmission_mn_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        return self.get_simple_element(element_id=TransId.spec_shift, default_value="")

    @transmission_mn_shift.setter
    def transmission_mn_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        self.set_simple_element(element_id=TransId.spec_shift, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Fit
    # ------------------------------------------------------------------------------------------------------------------
    def _get_transmission_fit(self, data_type, attribute, default_value):
        if FitId.general in self._user_file_items:
            settings = self._user_file_items[FitId.general]
            # Check first if there are data type specific settings, else check if there are general settings
            extracted_settings = [setting for setting in settings if setting.data_type is data_type]
            if not extracted_settings:
                extracted_settings = [setting for setting in settings if setting.data_type is None]
            if extracted_settings:
                setting = extracted_settings[-1]
                return getattr(setting, attribute)
        return default_value

    def _set_transmission_fit(self, data_type, start=None, stop=None, fit_type=None, polynomial_order=None):
        if FitId.general in self._user_file_items:
            # Gather all settings which correspond to the data type and where the data type is none
            settings = self._user_file_items[FitId.general]
            settings_general = [setting for setting in settings if setting.data_type is None]
            settings_for_data_type = [setting for setting in settings if setting.data_type is data_type]
            # We check if there are data-type specific settings.
            # 1. There are data type specific settings. Then we are good.
            # 2. There are no data type specific settings. We create one data type specific setting and populate it
            #    with a general setting if it exists else we create a new entry
            if not settings_for_data_type:
                if settings_general:
                    setting_general = settings_general[-1]
                    settings.append(fit_general(start=setting_general.start, stop=setting_general.stop,
                                                data_type=data_type, fit_type=setting_general.fit_type,
                                                polynomial_order=setting_general.polynomial_order))
                else:
                    settings.append(fit_general(start=None, stop=None, data_type=data_type,
                                                fit_type=FitType.NoFit, polynomial_order=2))
        else:
            settings = [fit_general(start=None, stop=None, data_type=data_type,
                                    fit_type=FitType.NoFit, polynomial_order=2)]

        new_settings = []
        for setting in settings:
            # We only want to modify the settings which are either the data type specific ones or the ones which
            # don't have a specific data type
            if setting.data_type is data_type and setting.data_type is not None:
                new_start = start if start is not None else setting.start
                new_stop = stop if stop is not None else setting.stop
                new_fit_type = fit_type if fit_type is not None else setting.fit_type
                new_polynomial_order = polynomial_order if polynomial_order is not None else setting.polynomial_order
                new_settings.append(fit_general(start=new_start, stop=new_stop, fit_type=new_fit_type,
                                                data_type=setting.data_type, polynomial_order=new_polynomial_order))
            else:
                new_settings.append(setting)
        self._user_file_items.update({FitId.general: new_settings})

    def has_transmission_fit_got_separate_settings_for_sample_and_can(self):
        if FitId.general in self._user_file_items:
            settings = self._user_file_items[FitId.general]
            if settings:
                settings_sample = [setting for setting in settings if setting.data_type is DataType.Sample]
                settings_can = [setting for setting in settings if setting.data_type is DataType.Can]
                # If we have either one or the other
                if settings_sample or settings_can:
                    return True
        return False

    @property
    def transmission_sample_fit_type(self):
        return self._get_transmission_fit(data_type=DataType.Sample, attribute="fit_type", default_value=FitType.NoFit)

    @transmission_sample_fit_type.setter
    def transmission_sample_fit_type(self, value):
        self._set_transmission_fit(data_type=DataType.Sample, fit_type=value)

    @property
    def transmission_can_fit_type(self):
        return self._get_transmission_fit(data_type=DataType.Can, attribute="fit_type", default_value=FitType.NoFit)

    @transmission_can_fit_type.setter
    def transmission_can_fit_type(self, value):
        self._set_transmission_fit(data_type=DataType.Can, fit_type=value)

    @property
    def transmission_sample_polynomial_order(self):
        return self._get_transmission_fit(data_type=DataType.Sample, attribute="polynomial_order",
                                          default_value=2)

    @transmission_sample_polynomial_order.setter
    def transmission_sample_polynomial_order(self, value):
        self._set_transmission_fit(data_type=DataType.Sample, polynomial_order=value)

    @property
    def transmission_can_polynomial_order(self):
        return self._get_transmission_fit(data_type=DataType.Can, attribute="polynomial_order",
                                          default_value=2)

    @transmission_can_polynomial_order.setter
    def transmission_can_polynomial_order(self, value):
        self._set_transmission_fit(data_type=DataType.Can, polynomial_order=value)

    @property
    def transmission_sample_wavelength_min(self):
        return self._get_transmission_fit(data_type=DataType.Sample, attribute="start", default_value="")

    @transmission_sample_wavelength_min.setter
    def transmission_sample_wavelength_min(self, value):
        self._set_transmission_fit(data_type=DataType.Sample, start=value)

    @property
    def transmission_sample_wavelength_max(self):
        return self._get_transmission_fit(data_type=DataType.Sample, attribute="stop", default_value="")

    @transmission_sample_wavelength_max.setter
    def transmission_sample_wavelength_max(self, value):
        self._set_transmission_fit(data_type=DataType.Sample, stop=value)

    @property
    def transmission_can_wavelength_min(self):
        return self._get_transmission_fit(data_type=DataType.Can, attribute="start", default_value="")

    @transmission_can_wavelength_min.setter
    def transmission_can_wavelength_min(self, value):
        self._set_transmission_fit(data_type=DataType.Can, start=value)

    @property
    def transmission_can_wavelength_max(self):
        return self._get_transmission_fit(data_type=DataType.Can, attribute="stop", default_value="")

    @transmission_can_wavelength_max.setter
    def transmission_can_wavelength_max(self, value):
        self._set_transmission_fit(data_type=DataType.Can, stop=value)

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

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q Limits
    # ------------------------------------------------------------------------------------------------------------------
    def _set_q_1d_limits(self, min_value=None, max_value=None, rebin_string=None):
        element_id = LimitsId.q
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
        element_id = LimitsId.qxy
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
        return self.get_simple_element_with_attribute(element_id=LimitsId.q, default_value="",
                                                      attribute="rebin_string")

    @q_1d_rebin_string.setter
    def q_1d_rebin_string(self, value):
        self._set_q_1d_limits(rebin_string=value)

    @property
    def q_xy_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.qxy, default_value="",
                                                      attribute="stop")

    @q_xy_max.setter
    def q_xy_max(self, value):
        self._set_q_xy_limits(stop_value=value)

    @property
    def q_xy_step(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.qxy, default_value="",
                                                      attribute="step")

    @q_xy_step.setter
    def q_xy_step(self, value):
        self._set_q_xy_limits(step_value=value)

    @property
    def q_xy_step_type(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.qxy, default_value=None,
                                                      attribute="step_type")

    @q_xy_step_type.setter
    def q_xy_step_type(self, value):
        self._set_q_xy_limits(step_type_value=value)

    @property
    def r_cut(self):
        return self.get_simple_element(element_id=LimitsId.radius_cut, default_value="")

    @r_cut.setter
    def r_cut(self, value):
        self.set_simple_element(element_id=LimitsId.radius_cut, value=value)

    @property
    def w_cut(self):
        return self.get_simple_element(element_id=LimitsId.wavelength_cut, default_value="")

    @w_cut.setter
    def w_cut(self, value):
        self.set_simple_element(element_id=LimitsId.wavelength_cut, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def gravity_on_off(self):
        return self.get_simple_element(element_id=GravityId.on_off, default_value=True)

    @gravity_on_off.setter
    def gravity_on_off(self, value):
        self.set_simple_element(element_id=GravityId.on_off, value=value)

    @property
    def gravity_extra_length(self):
        return self.get_simple_element(element_id=GravityId.extra_length, default_value="")

    @gravity_extra_length.setter
    def gravity_extra_length(self, value):
        self.set_simple_element(element_id=GravityId.extra_length, value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # QResolution
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def use_q_resolution(self):
        return self.get_simple_element(element_id=QResolutionId.on, default_value=False)

    @use_q_resolution.setter
    def use_q_resolution(self, value):
        self.set_simple_element(element_id=QResolutionId.on, value=value)

    @property
    def q_resolution_source_a(self):
        return self.get_simple_element(element_id=QResolutionId.a1, default_value="")

    @q_resolution_source_a.setter
    def q_resolution_source_a(self, value):
        self.set_simple_element(element_id=QResolutionId.a1, value=value)

    @property
    def q_resolution_sample_a(self):
        return self.get_simple_element(element_id=QResolutionId.a2, default_value="")

    @q_resolution_sample_a.setter
    def q_resolution_sample_a(self, value):
        self.set_simple_element(element_id=QResolutionId.a2, value=value)

    @property
    def q_resolution_source_h(self):
        return self.get_simple_element(element_id=QResolutionId.h1, default_value="")

    @q_resolution_source_h.setter
    def q_resolution_source_h(self, value):
        self.set_simple_element(element_id=QResolutionId.h1, value=value)

    @property
    def q_resolution_sample_h(self):
        return self.get_simple_element(element_id=QResolutionId.h2, default_value="")

    @q_resolution_sample_h.setter
    def q_resolution_sample_h(self, value):
        self.set_simple_element(element_id=QResolutionId.h2, value=value)

    @property
    def q_resolution_source_w(self):
        return self.get_simple_element(element_id=QResolutionId.w1, default_value="")

    @q_resolution_source_w.setter
    def q_resolution_source_w(self, value):
        self.set_simple_element(element_id=QResolutionId.w1, value=value)

    @property
    def q_resolution_sample_w(self):
        return self.get_simple_element(element_id=QResolutionId.w2, default_value="")

    @q_resolution_sample_w.setter
    def q_resolution_sample_w(self, value):
        self.set_simple_element(element_id=QResolutionId.w2, value=value)

    @property
    def q_resolution_delta_r(self):
        return self.get_simple_element(element_id=QResolutionId.delta_r, default_value="")

    @q_resolution_delta_r.setter
    def q_resolution_delta_r(self, value):
        self.set_simple_element(element_id=QResolutionId.delta_r, value=value)

    @property
    def q_resolution_moderator_file(self):
        return self.get_simple_element(element_id=QResolutionId.moderator, default_value="")

    @q_resolution_moderator_file.setter
    def q_resolution_moderator_file(self, value):
        self.set_simple_element(element_id=QResolutionId.moderator, value=value)

    @property
    def q_resolution_collimation_length(self):
        return self.get_simple_element(element_id=QResolutionId.collimation_length, default_value="")

    @q_resolution_collimation_length.setter
    def q_resolution_collimation_length(self, value):
        self.set_simple_element(element_id=QResolutionId.collimation_length, value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi limit
    # ------------------------------------------------------------------------------------------------------------------
    def _set_phi_limit(self, min_value=None, max_value=None, use_mirror=None):
        if LimitsId.angle in self._user_file_items:
            settings = self._user_file_items[LimitsId.angle]
        else:
            settings = [mask_angle_entry(min=None, max=None, use_mirror=False)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value is not None else setting.min
            new_max = max_value if max_value is not None else setting.max
            new_use_mirror = use_mirror if use_mirror is not None else setting.use_mirror
            new_settings.append(mask_angle_entry(min=new_min, max=new_max, use_mirror=new_use_mirror))
        self._user_file_items.update({LimitsId.angle: new_settings})

    @property
    def phi_limit_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.angle, attribute="min", default_value="-90")

    @phi_limit_min.setter
    def phi_limit_min(self, value):
        self._set_phi_limit(min_value=value)

    @property
    def phi_limit_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.angle, attribute="max", default_value="90")

    @phi_limit_max.setter
    def phi_limit_max(self, value):
        self._set_phi_limit(max_value=value)

    @property
    def phi_limit_use_mirror(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.angle, attribute="use_mirror", default_value=True)  # noqa

    @phi_limit_use_mirror.setter
    def phi_limit_use_mirror(self, value):
        self._set_phi_limit(use_mirror=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Radius limit
    # ------------------------------------------------------------------------------------------------------------------
    def _set_radius_limit(self, min_value=None, max_value=None):
        if LimitsId.radius in self._user_file_items:
            settings = self._user_file_items[LimitsId.radius]
        else:
            settings = [range_entry(start=None, stop=None)]

        new_settings = []
        for setting in settings:
            new_min = min_value if min_value is not None else setting.start
            new_max = max_value if max_value is not None else setting.stop
            new_settings.append(range_entry(start=new_min, stop=new_max))
        self._user_file_items.update({LimitsId.radius: new_settings})

    @property
    def radius_limit_min(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.radius, attribute="start", default_value="")

    @radius_limit_min.setter
    def radius_limit_min(self, value):
        self._set_radius_limit(min_value=value)

    @property
    def radius_limit_max(self):
        return self.get_simple_element_with_attribute(element_id=LimitsId.radius, attribute="stop", default_value="")

    @radius_limit_max.setter
    def radius_limit_max(self, value):
        self._set_radius_limit(max_value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Mask files
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def mask_files(self):
        if MaskId.file in self._user_file_items:
            return self._user_file_items[MaskId.file]
        return []

    @mask_files.setter
    def mask_files(self, value):
        if value is None:
            return
        if MaskId.file in self._user_file_items:
            del self._user_file_items[MaskId.file]
        new_state_entries = {MaskId.file: value}
        self._user_file_items.update(new_state_entries)

    # ------------------------------------------------------------------------------------------------------------------
    # Output name
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def output_name(self):
        return self.get_simple_element(element_id=OtherId.user_specified_output_name, default_value="")

    @output_name.setter
    def output_name(self, value):
        self.set_simple_element(element_id=OtherId.user_specified_output_name, value=value)
