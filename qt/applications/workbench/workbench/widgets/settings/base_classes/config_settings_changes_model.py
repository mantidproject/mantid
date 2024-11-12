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
        self._changes: Dict[str, str] = {}

    def properties_to_be_changed(self) -> List[str]:
        return list(self._changes.keys())

    def has_unsaved_changes(self) -> bool:
        return self._changes != {}

    def get_changes(self) -> Dict[str, str]:
        return self._changes

    def apply_changes(self) -> None:
        for property_string, value in self._changes.items():
            ConfigService.setString(property_string, value)
        self._changes.clear()

    def add_change(self, property_string: str, value: str) -> None:
        saved_value = self.get_saved_value(property_string)
        if saved_value != value:
            self._changes[property_string] = value
        elif property_string in self._changes.keys():
            self._changes.pop(property_string)

    @staticmethod
    def get_saved_value(property_string: str) -> str:
        return ConfigService.getString(property_string)
