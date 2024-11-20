# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import ast
from typing import List, Dict, Any

from mantid import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.general.general_settings_model import GeneralUserConfigProperties
from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


class SettingsModel:
    def __init__(self, category_setting_models: List[ConfigSettingsChangesModel] = None):
        self.category_setting_models: List[ConfigSettingsChangesModel] = category_setting_models
        self.properties_which_need_a_restart = []

    def register_property_which_needs_a_restart(self, property_string: str) -> None:
        self.properties_which_need_a_restart.append(property_string)

    def potential_changes_that_need_a_restart(self) -> List[str]:
        def search_for_property_in_all_model_changes(property_string: str) -> bool:
            for model in self.category_setting_models:
                if property_string in model.properties_to_be_changed():
                    return True
            return False

        return [
            property_string
            for property_string in self.properties_which_need_a_restart
            if search_for_property_in_all_model_changes(property_string)
        ]

    def apply_all_settings(self) -> None:
        for model in self.category_setting_models:
            model.apply_changes()

    def unsaved_changes(self) -> Dict[str, Any]:
        changes = {}
        for model in self.category_setting_models:
            changes |= model.get_changes()

        return changes

    @staticmethod
    def save_settings_to_file(filepath, settings):
        with open(filepath, "w") as file:
            for setting in settings:
                if CONF.has(setting):
                    if setting != GeneralUserConfigProperties.USER_LAYOUT.value:
                        value = CONF.get(setting, type=str)
                else:
                    value = ConfigService.getString(setting)
                file.write(setting + "=" + str(value) + "\n")

    @staticmethod
    def load_settings_from_file(filepath, settings):
        with open(filepath, "r") as file:
            line = file.readline()
            while line:
                setting = line.rstrip("\n").split("=", 1)
                prop = setting[0].strip()
                if prop in settings:
                    if CONF.has(prop) or prop.find("/") != -1:
                        if prop == GeneralUserConfigProperties.USER_LAYOUT.value:
                            pass
                        elif prop == GeneralUserConfigProperties.FONT.value and not setting[1]:
                            # if the font setting is empty removing it will make workbench use the default
                            CONF.remove(prop)
                        else:
                            try:
                                value = ast.literal_eval(setting[1])
                            except (SyntaxError, ValueError):
                                value = setting[1]
                            CONF.set(setting[0].strip(), value)
                    else:
                        ConfigService.setString(prop, setting[1])
                line = file.readline()
