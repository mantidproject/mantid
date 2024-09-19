# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import ConfigService
from SANS.sans.common.enums import BinningType


class OverlayEventWorkspaces(object):
    def __init__(self, enabled_by_default):
        self._overlay_event_workspaces = enabled_by_default

    def is_overlay_event_workspaces_enabled(self):
        return self._overlay_event_workspaces

    def enable_overlay_event_workspaces(self):
        self._overlay_event_workspaces = True

    def disable_overlay_event_workspaces(self):
        self._overlay_event_workspaces = False


class CustomBinning(object):
    def __init__(self):
        self._bin_settings = ""

    @property
    def bin_settings(self):
        return self._bin_settings

    @bin_settings.setter
    def bin_settings(self, bin_settings):
        self._bin_settings = bin_settings

    def has_bin_settings(self):
        return True

    def has_additional_time_shifts(self):
        return False

    def has_overlay_event_workspaces(self):
        return False


class SaveAsEventData(OverlayEventWorkspaces):
    def __init__(self):
        super(SaveAsEventData, self).__init__(False)
        self._additional_time_shifts = ""

    @property
    def additional_time_shifts(self):
        return self._additional_time_shifts

    @additional_time_shifts.setter
    def additional_time_shifts(self, additional_time_shifts):
        self._additional_time_shifts = additional_time_shifts

    def has_bin_settings(self):
        return False

    def has_additional_time_shifts(self):
        return self._overlay_event_workspaces

    def has_overlay_event_workspaces(self):
        return True


class BinningFromMonitors(object):
    def has_bin_settings(self):
        return False

    def has_additional_time_shifts(self):
        return False

    def has_overlay_event_workspaces(self):
        return False


class SummationSettingsModel(object):
    def __init__(self, initial_type):
        self._save_directory = ConfigService.getString("defaultsave.directory")
        self._type_factory_dict = {
            BinningType.SAVE_AS_EVENT_DATA: SaveAsEventData(),
            BinningType.CUSTOM: CustomBinning(),
            BinningType.FROM_MONITORS: BinningFromMonitors(),
        }
        self._settings, self._type = self._settings_from_type(initial_type)

    def instrument(self):
        return ConfigService.getString("default.instrument")

    @property
    def save_directory(self):
        return self._save_directory

    @save_directory.setter
    def save_directory(self, directory):
        self._save_directory = directory

    def set_histogram_binning_type(self, type):
        self._settings, self._type = self._settings_from_type(type)

    def has_bin_settings(self):
        return self._settings.has_bin_settings()

    def has_additional_time_shifts(self):
        return self._settings.has_additional_time_shifts()

    def has_overlay_event_workspaces(self):
        return self._settings.has_overlay_event_workspaces()

    def should_save_as_event_workspaces(self):
        return isinstance(self._settings, SaveAsEventData)

    @property
    def type(self):
        return self._type

    @property
    def additional_time_shifts(self):
        return self._settings.additional_time_shifts

    @additional_time_shifts.setter
    def additional_time_shifts(self, additional_time_shifts):
        self._settings.additional_time_shifts = additional_time_shifts

    def enable_overlay_event_workspaces(self):
        self._settings.enable_overlay_event_workspaces()

    def disable_overlay_event_workspaces(self):
        self._settings.disable_overlay_event_workspaces()

    def is_overlay_event_workspaces_enabled(self):
        return self._settings.is_overlay_event_workspaces_enabled()

    @property
    def bin_settings(self):
        return self._settings.bin_settings

    @bin_settings.setter
    def bin_settings(self, bin_settings):
        self._settings.bin_settings = bin_settings

    def _settings_from_type(self, type):
        try:
            return self._type_factory_dict[type], type
        except KeyError:
            # default
            return self._type_factory_dict[BinningType.CUSTOM], BinningType.CUSTOM
