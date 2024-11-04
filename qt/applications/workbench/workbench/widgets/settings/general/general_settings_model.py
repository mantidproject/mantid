# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from enum import Enum
from typing import List, Any, Dict

from mantid.kernel import ConfigService

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel
from workbench.config import CONF


class GeneralProperties(Enum):
    CRYSTALLOGRAPY_CONV = "Q.convention"
    FACILITY = "default.facility"
    INSTRUMENT = "default.instrument"
    OPENGL = "MantidOptions.InstrumentView.UseOpenGL"
    SHOW_INVISIBLE_WORKSPACES = "MantidOptions.InvisibleWorkspaces"
    PR_NUMBER_OF_CHECKPOINTS = "projectRecovery.numberOfCheckpoints"
    PR_TIME_BETWEEN_RECOVERY = "projectRecovery.secondsBetween"
    PR_RECOVERY_ENABLED = "projectRecovery.enabled"
    USE_NOTIFICATIONS = "Notifications.Enabled"


class GeneralUserConfigProperties(Enum):
    FONT = "MainWindow/font"
    PROMPT_ON_DELETING_WORKSPACE = "project/prompt_on_deleting_workspace"
    PROMPT_SAVE_EDITOR_MODIFIED = "project/prompt_save_editor_modified"
    PROMPT_SAVE_ON_CLOSE = "project/prompt_save_on_close"
    USER_LAYOUT = "MainWindow/user_layouts"
    WINDOW_BEHAVIOUR = "AdditionalWindows/behaviour"
    COMPLETION_ENABLED = "Editors/completion_enabled"


class GeneralSettingsModel(ConfigSettingsChangesModel):
    def __init__(self):
        super().__init__()
        self.user_config_changes: Dict[str, Any] = {}

    def has_unsaved_changes(self) -> bool:
        return self.user_config_changes != {} or super().has_unsaved_changes()

    def apply_changes(self) -> None:
        for property_string, value in self.user_config_changes:
            CONF.set(property_string, value)
        self.user_config_changes.clear()

        if GeneralProperties.FACILITY.value in self.changes:
            facility_value = self.changes.pop(GeneralProperties.FACILITY.value)
            # Go through setFacility because it does some checking and updates the instrument.
            ConfigService.setFacility(facility_value)
        super().apply_changes()

    def add_change(self, property_string: str, value: Any) -> None:
        if property_string in [e.value for e in GeneralUserConfigProperties]:
            saved_value = None
            if CONF.has(property_string):
                saved_value = CONF.get(property_string, type=type(value))

            if saved_value != value:
                self.user_config_changes[property_string] = value
            elif property_string in self.user_config_changes.keys():
                self.user_config_changes.pop(property_string)
        else:
            super().add_change(property_string, value)

    def get_crystallography_convention(self) -> str:
        return self.get_saved_value(GeneralProperties.CRYSTALLOGRAPY_CONV.value)

    def get_facility(self) -> str:
        return self.get_saved_value(GeneralProperties.FACILITY.value)

    @staticmethod
    def get_font() -> str | None:
        if CONF.has(GeneralUserConfigProperties.FONT.value):
            return CONF.get(GeneralUserConfigProperties.FONT.value)
        return None

    @staticmethod
    def get_facility_names() -> List[str]:
        return ConfigService.getFacilityNames()

    def get_instrument(self) -> str:
        return self.get_saved_value(GeneralProperties.INSTRUMENT.value)

    def get_use_opengl(self) -> str:
        return self.get_saved_value(GeneralProperties.OPENGL.value)

    def get_show_invisible_workspaces(self) -> str:
        return self.get_saved_value(GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value)

    def get_project_recovery_number_of_checkpoints(self) -> str:
        return self.get_saved_value(GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value)

    def get_project_recovery_time_between_recoveries(self) -> str:
        return self.get_saved_value(GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value)

    def get_project_recovery_enabled(self) -> str:
        return self.get_saved_value(GeneralProperties.PR_RECOVERY_ENABLED.value)

    @staticmethod
    def get_prompt_on_deleting_workspace() -> bool:
        return CONF.get(GeneralUserConfigProperties.PROMPT_ON_DELETING_WORKSPACE.value, type=bool)

    @staticmethod
    def get_prompt_on_save_editor_modified() -> bool:
        return CONF.get(GeneralUserConfigProperties.PROMPT_SAVE_EDITOR_MODIFIED.value, type=bool)

    @staticmethod
    def get_prompt_save_on_close() -> bool:
        return CONF.get(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, type=bool)

    def get_use_notifications(self) -> str:
        return self.get_saved_value(GeneralProperties.USE_NOTIFICATIONS.value)

    def get_user_layout(self, get_potential_update=False) -> dict:
        if get_potential_update and GeneralUserConfigProperties.USER_LAYOUT.value in self.user_config_changes:
            return self.user_config_changes[GeneralUserConfigProperties.USER_LAYOUT.value]
        return CONF.get(GeneralUserConfigProperties.USER_LAYOUT.value, type=dict)

    @staticmethod
    def get_window_behaviour() -> str:
        return CONF.get(GeneralUserConfigProperties.WINDOW_BEHAVIOUR.value, type=str)

    @staticmethod
    def get_completion_enabled() -> str:
        return CONF.get(GeneralUserConfigProperties.COMPLETION_ENABLED.value, type=bool)

    def set_crystallography_convention(self, value: str) -> None:
        self.add_change(GeneralProperties.CRYSTALLOGRAPY_CONV.value, value)

    def set_facility(self, value: str) -> None:
        self.add_change(GeneralProperties.FACILITY.value, value)

    def set_font(self, value: str) -> None:
        self.add_change(GeneralUserConfigProperties.FONT.value, value)

    def set_window_behaviour(self, value: str) -> None:
        self.add_change(GeneralUserConfigProperties.WINDOW_BEHAVIOUR.value, value)

    def set_completion_enabled(self, value: bool) -> None:
        self.add_change(GeneralUserConfigProperties.COMPLETION_ENABLED.value, value)

    def set_prompt_save_on_close(self, value: bool) -> None:
        self.add_change(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, value)

    def set_prompt_on_save_editor_modified(self, value: bool) -> None:
        self.add_change(GeneralUserConfigProperties.PROMPT_SAVE_EDITOR_MODIFIED.value, value)

    def set_prompt_on_deleting_workspace(self, value: bool) -> None:
        self.add_change(GeneralUserConfigProperties.PROMPT_ON_DELETING_WORKSPACE.value, value)

    def set_use_notifications(self, value: str) -> None:
        self.add_change(GeneralProperties.USE_NOTIFICATIONS.value, value)

    def set_project_recovery_enabled(self, value: str) -> None:
        self.add_change(GeneralProperties.PR_RECOVERY_ENABLED.value, value)

    def set_project_recovery_time_between_recoveries(self, value: str) -> None:
        self.add_change(GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value, value)

    def set_project_recovery_number_of_checkpoints(self, value: str) -> None:
        self.add_change(GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value, value)

    def set_instrument(self, value: str) -> None:
        self.add_change(GeneralProperties.INSTRUMENT.value, value)

    def set_show_invisible_workspaces(self, value: str) -> None:
        self.add_change(GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value, value)

    def set_use_opengl(self, value: str) -> None:
        self.add_change(GeneralProperties.OPENGL.value, value)

    def set_user_layout(self, value: dict) -> None:
        self.add_change(GeneralUserConfigProperties.USER_LAYOUT.value, value)
