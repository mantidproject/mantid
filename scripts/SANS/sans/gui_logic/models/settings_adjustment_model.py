# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The state gui model contains all the reduction information which is not explicitly available in the data table.

This is one of the two models which is used for the data reduction. It contains generally all the settings which
are not available in the model associated with the data table.
"""

from __future__ import (absolute_import, division, print_function)

from sans.common.enums import SANSInstrument, DataType, FitType, DetectorType
from sans.gui_logic.models.model_common import ModelCommon
from sans.user_file.settings_tags import TransId, MonId, FitId, fit_general, monitor_file


class SettingsAdjustmentModel(ModelCommon):

    def __init__(self, user_file_items=None):
        super(SettingsAdjustmentModel, self).__init__(user_file_items=user_file_items)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    # ================== Logic =======================
    def does_instrument_support_monitor_5(self):
        """
        Checks if the current instrument supports monitor 5
        :return: True if the instrument does
        """
        # At a later date we could programmatically determine if an instrument is using
        # (or supports) monitor 5 by summing the counts and enabling where != 0
        # for the moment however only Zoom has this capability so just hard code check
        return True if self.instrument is SANSInstrument.ZOOM else False

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

    # =================== Property helper methods ================

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

    # ================= Property managers ============

    # ------------------------------------------------------------------------------------------------------------------
    # Adjustment settings
    # ------------------------------------------------------------------------------------------------------------------

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

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission Fitting
    # ------------------------------------------------------------------------------------------------------------------

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
    # Monitor normalization
    # ------------------------------------------------------------------------------------------------------------------

    @property
    def normalization_interpolate(self):
        return self._get_incident_spectrum_info(default_value=False, attribute="interpolate", is_trans=False)

    @normalization_interpolate.setter
    def normalization_interpolate(self, value):
        self._update_incident_spectrum_info(interpolate=value, is_trans=False)

    @property
    def normalization_incident_monitor(self):
        return self._get_incident_spectrum_info(default_value="", attribute="spectrum", is_trans=False)

    @normalization_incident_monitor.setter
    def normalization_incident_monitor(self, value):
        self._update_incident_spectrum_info(spectrum=value, is_trans=False)

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
    def transmission_mn_4_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        return self.get_simple_element(element_id=TransId.spec_4_shift, default_value="")

    @transmission_mn_4_shift.setter
    def transmission_mn_4_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        self.set_simple_element(element_id=TransId.spec_4_shift, value=value)

    @property
    def transmission_mn_5_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        return self.get_simple_element(element_id=TransId.spec_5_shift, default_value="")

    @transmission_mn_5_shift.setter
    def transmission_mn_5_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        self.set_simple_element(element_id=TransId.spec_5_shift, value=value)
