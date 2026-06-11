# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QSettings
from typing import Dict, Any, Type

INTERFACES_SETTINGS_GROUP = "CustomInterfaces"
TEXTURE_PLANNER_PREFIX = "TexturePlanner/"

SETTINGS_DICT = {
    "directions": bool,
    "goniometers": bool,
    "incident": bool,
    "ks": bool,
    "scattered": bool,
    "stl_scale": str,
    "stl_x_degrees": float,
    "stl_y_degrees": float,
    "stl_z_degrees": float,
    "stl_translation_vector": str,
    "orientation_axes": str,
    "orientation_senses": str,
    "mc_events_per_point": int,
    "mc_max_scatter_attempts": int,
    "mc_simulate_in": str,
    "mc_resimulate": bool,
    "att_point": float,
    "att_unit": str,
    "att_use_data_range": bool,
}

DEFAULT_SETTINGS = {
    "directions": True,
    "goniometers": True,
    "incident": True,
    "ks": True,
    "scattered": False,
    "stl_scale": "cm",
    "stl_x_degrees": 0.0,
    "stl_y_degrees": 0.0,
    "stl_z_degrees": 0.0,
    "stl_translation_vector": "0,0,0",
    "orientation_axes": "YXY",
    "orientation_senses": "-1,-1,-1",
    "mc_events_per_point": 50,
    "mc_max_scatter_attempts": 10000,
    "mc_simulate_in": "SampleOnly",
    "mc_resimulate": False,
    "att_point": 1.5,
    "att_unit": "dSpacing",
    "att_use_data_range": False,
}


class TexturePlannerSettingsModel:
    def get_settings_dict(self) -> Dict[str, Any]:
        """Load all settings from QSettings, falling back to defaults for any missing entry."""
        settings = {}
        for name, return_type in SETTINGS_DICT.items():
            value = self._get_setting(name, return_type)
            settings[name] = value if (value != "" and value is not None) else DEFAULT_SETTINGS[name]
        return settings

    def set_settings_dict(self, settings: Dict[str, Any]) -> None:
        """Persist all settings to QSettings."""
        for name, value in settings.items():
            if name in SETTINGS_DICT:
                self._set_setting(name, value)

    @staticmethod
    def _get_setting(name: str, return_type: Type = str) -> Any:
        qs = QSettings()
        qs.beginGroup(INTERFACES_SETTINGS_GROUP)
        if return_type is bool:
            raw = qs.value(TEXTURE_PLANNER_PREFIX + name, type=str)
            if raw == "true":
                value = True
            elif raw == "false":
                value = False
            else:
                value = ""
        else:
            value = qs.value(TEXTURE_PLANNER_PREFIX + name, type=return_type)
        qs.endGroup()
        return value

    @staticmethod
    def _set_setting(name: str, value: Any) -> None:
        qs = QSettings()
        qs.beginGroup(INTERFACES_SETTINGS_GROUP)
        qs.setValue(TEXTURE_PLANNER_PREFIX + name, value)
        qs.endGroup()
