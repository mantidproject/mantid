# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, MagicMock, call

from mantidqtinterfaces.TexturePlanner.settings.settings_model import (
    DEFAULT_SETTINGS,
    INTERFACES_SETTINGS_GROUP,
    SETTINGS_DICT,
    TEXTURE_PLANNER_PREFIX,
    TexturePlannerSettingsModel,
)

file_path = "mantidqtinterfaces.TexturePlanner.settings.settings_model"


class TestTexturePlannerSettingsModel_GetSetting(unittest.TestCase):
    @patch(file_path + ".QSettings")
    def test_non_bool_returns_qsettings_value_typed(self, mock_qsettings):
        qs = MagicMock()
        qs.value.return_value = 1.5
        mock_qsettings.return_value = qs

        result = TexturePlannerSettingsModel._get_setting("att_point", float)

        qs.beginGroup.assert_called_once_with(INTERFACES_SETTINGS_GROUP)
        qs.value.assert_called_once_with(TEXTURE_PLANNER_PREFIX + "att_point", type=float)
        qs.endGroup.assert_called_once_with()
        self.assertEqual(result, 1.5)

    @patch(file_path + ".QSettings")
    def test_bool_true_string_returns_true(self, mock_qsettings):
        qs = MagicMock()
        qs.value.return_value = "true"
        mock_qsettings.return_value = qs

        result = TexturePlannerSettingsModel._get_setting("directions", bool)

        qs.value.assert_called_once_with(TEXTURE_PLANNER_PREFIX + "directions", type=str)
        self.assertIs(result, True)

    @patch(file_path + ".QSettings")
    def test_bool_false_string_returns_false(self, mock_qsettings):
        qs = MagicMock()
        qs.value.return_value = "false"
        mock_qsettings.return_value = qs

        result = TexturePlannerSettingsModel._get_setting("directions", bool)

        self.assertIs(result, False)

    @patch(file_path + ".QSettings")
    def test_bool_missing_returns_empty_string_sentinel(self, mock_qsettings):
        qs = MagicMock()
        qs.value.return_value = ""
        mock_qsettings.return_value = qs

        result = TexturePlannerSettingsModel._get_setting("directions", bool)

        self.assertEqual(result, "")


class TestTexturePlannerSettingsModel_SetSetting(unittest.TestCase):
    @patch(file_path + ".QSettings")
    def test_sets_value_under_prefixed_key(self, mock_qsettings):
        qs = MagicMock()
        mock_qsettings.return_value = qs

        TexturePlannerSettingsModel._set_setting("att_material", "Cu")

        qs.beginGroup.assert_called_once_with(INTERFACES_SETTINGS_GROUP)
        qs.setValue.assert_called_once_with(TEXTURE_PLANNER_PREFIX + "att_material", "Cu")
        qs.endGroup.assert_called_once_with()


class TestTexturePlannerSettingsModel_GetSettingsDict(unittest.TestCase):
    def test_returns_value_from_get_setting_for_each_key(self):
        model = TexturePlannerSettingsModel()
        canned = {name: (True if t is bool else t(1) if t in (int, float) else "x") for name, t in SETTINGS_DICT.items()}

        with patch.object(TexturePlannerSettingsModel, "_get_setting", side_effect=lambda name, t: canned[name]):
            result = model.get_settings_dict()

        self.assertEqual(result, canned)

    def test_falls_back_to_default_when_value_is_empty_string(self):
        model = TexturePlannerSettingsModel()

        with patch.object(TexturePlannerSettingsModel, "_get_setting", return_value=""):
            result = model.get_settings_dict()

        self.assertEqual(result, DEFAULT_SETTINGS)

    def test_falls_back_to_default_when_value_is_none(self):
        model = TexturePlannerSettingsModel()

        with patch.object(TexturePlannerSettingsModel, "_get_setting", return_value=None):
            result = model.get_settings_dict()

        self.assertEqual(result, DEFAULT_SETTINGS)


class TestTexturePlannerSettingsModel_SetSettingsDict(unittest.TestCase):
    def test_writes_each_known_setting_via_set_setting(self):
        model = TexturePlannerSettingsModel()
        payload = {"directions": False, "att_material": "Cu", "att_point": 2.0}

        with patch.object(TexturePlannerSettingsModel, "_set_setting") as mock_set:
            model.set_settings_dict(payload)

        self.assertEqual(
            sorted(mock_set.call_args_list),
            sorted(
                [
                    call("directions", False),
                    call("att_material", "Cu"),
                    call("att_point", 2.0),
                ]
            ),
        )

    def test_ignores_keys_not_in_settings_dict(self):
        model = TexturePlannerSettingsModel()
        payload = {"directions": True, "not_a_real_setting": 123}

        with patch.object(TexturePlannerSettingsModel, "_set_setting") as mock_set:
            model.set_settings_dict(payload)

        mock_set.assert_called_once_with("directions", True)


class TestTexturePlannerSettingsModel_DefaultsAlignment(unittest.TestCase):
    """DEFAULT_SETTINGS and SETTINGS_DICT must cover the same keys: a typed entry without a
    default would crash get_settings_dict, and a default without a type entry would never be
    read or written."""

    def test_default_settings_keys_match_settings_dict_keys(self):
        self.assertEqual(set(DEFAULT_SETTINGS.keys()), set(SETTINGS_DICT.keys()))


if __name__ == "__main__":
    unittest.main()
