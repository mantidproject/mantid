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
        self.apply_callbacks: List = []

    def apply_changes(self) -> None:
        for property_string, value in self.changes.items():
            ConfigService.setString(property_string, value)
        for callback in self.apply_callbacks:
            callback()

    def add_change(self, property_string: str, value: str) -> None:
        self.changes[property_string] = value

    @staticmethod
    def get_saved_value(property_string) -> str:
        return ConfigService.getString(property_string)

    def register_apply_callback(self, callback):
        self.apply_callbacks.append(callback)
