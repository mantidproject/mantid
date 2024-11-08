# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from typing import Dict, List

from mantid.kernel import ConfigService


class ConfigSettingsChangesModel:
    def __init__(self):
        self.changes: Dict[str, str] = {}

    def properties_to_be_changed(self) -> List[str]:
        return list(self.changes.keys())

    def has_unsaved_changes(self) -> bool:
        return self.changes != {}

    def get_changes_dict(self) -> Dict[str, str]:
        return self.changes

    def apply_changes(self) -> None:
        for property_string, value in self.changes.items():
            ConfigService.setString(property_string, value)
        self.changes.clear()

    def add_change(self, property_string: str, value: str) -> None:
        saved_value = self.get_saved_value(property_string)
        if saved_value != value:
            self.changes[property_string] = value
        elif property_string in self.changes.keys():
            self.changes.pop(property_string)

    @staticmethod
    def get_saved_value(property_string) -> str:
        return ConfigService.getString(property_string)
