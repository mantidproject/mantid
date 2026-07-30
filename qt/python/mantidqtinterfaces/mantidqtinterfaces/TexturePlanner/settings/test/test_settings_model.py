# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import tempfile
import unittest

from unittest.mock import patch, MagicMock, call

from qtpy.QtCore import QCoreApplication, QSettings

from mantidqtinterfaces.TexturePlanner.settings.settings_model import (
    DEFAULT_SETTINGS,
    INTERFACES_SETTINGS_GROUP,
    SETTINGS_DICT,
    TEXTURE_PLANNER_PREFIX,
    TexturePlannerSettingsModel,
)

file_path = "mantidqtinterfaces.TexturePlanner.settings.settings_model"


class TestTexturePlannerSettingsModel_GetSetting(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        QCoreApplication.setApplicationName("test_texture_planner")
        QCoreApplication.setOrganizationName("test_texture_planner_org")
        cls.settings_dir = tempfile.TemporaryDirectory()

    @classmethod
    def tearDownClass(cls):
        QSettings().clear()
        cls.settings_dir.cleanup()

    def setUp(self):
        QSettings().clear()

    @staticmethod
    def _write(name, value):
        qs = QSettings()
        qs.beginGroup(INTERFACES_SETTINGS_GROUP)
        qs.setValue(TEXTURE_PLANNER_PREFIX + name, value)
        qs.endGroup()

    def test_stored_value_round_trips_with_its_type(self):
        for name, value in (("att_point", 2.75), ("mc_events_per_point", 7), ("att_unit", "Wavelength"), ("directions", True)):
            with self.subTest(name=name):
                self._write(name, value)

                result = TexturePlannerSettingsModel._get_setting(name, SETTINGS_DICT[name], DEFAULT_SETTINGS[name])

                self.assertEqual(result, value)
                self.assertIsInstance(result, type(value))

    def test_numeric_value_that_cannot_be_converted_returns_default(self):
        for name in ("att_point", "mc_events_per_point"):
            for raw in ("garbage", ""):
                with self.subTest(name=name, raw=raw):
                    self._write(name, raw)

                    result = TexturePlannerSettingsModel._get_setting(name, SETTINGS_DICT[name], DEFAULT_SETTINGS[name])

                    self.assertEqual(result, DEFAULT_SETTINGS[name])

    def test_corrupted_bool_uses_default(self):
        default_value = True
        for raw, expected in (("tru", default_value), ("0", default_value), ("", default_value)):
            with self.subTest(raw=raw):
                self._write("directions", raw)

                self.assertIs(TexturePlannerSettingsModel._get_setting("directions", bool, default_value), expected)

    def test_missing_key_returns_default(self):
        for name in SETTINGS_DICT:
            with self.subTest(name=name):
                result = TexturePlannerSettingsModel._get_setting(name, SETTINGS_DICT[name], DEFAULT_SETTINGS[name])

                self.assertEqual(result, DEFAULT_SETTINGS[name])
                self.assertIsInstance(result, type(DEFAULT_SETTINGS[name]))

    def test_value_is_read_from_the_texture_planner_group_only(self):
        # Same setting name, written outside the interface's own group and prefix.
        qs = QSettings()
        qs.setValue("att_unit", "Wavelength")
        qs.beginGroup(INTERFACES_SETTINGS_GROUP)
        qs.setValue("att_unit", "TOF")
        qs.endGroup()

        self.assertEqual(TexturePlannerSettingsModel._get_setting("att_unit", str, "dSpacing"), "dSpacing")

    @patch(file_path + ".QSettings")
    def test_missing_key_is_not_looked_up(self, mock_qsettings):
        qs = MagicMock()
        qs.contains.return_value = False
        mock_qsettings.return_value = qs

        result = TexturePlannerSettingsModel._get_setting("att_point", float, 1.5)

        qs.beginGroup.assert_called_once_with(INTERFACES_SETTINGS_GROUP)
        qs.contains.assert_called_once_with(TEXTURE_PLANNER_PREFIX + "att_point")
        qs.value.assert_not_called()
        qs.endGroup.assert_called_once_with()
        self.assertEqual(result, 1.5)


class TestTexturePlannerSettingsModel_SetSetting(unittest.TestCase):
    @patch(file_path + ".QSettings")
    def test_sets_value_under_prefixed_key(self, mock_qsettings):
        qs = MagicMock()
        mock_qsettings.return_value = qs

        TexturePlannerSettingsModel._set_setting("att_unit", "Wavelength")

        qs.beginGroup.assert_called_once_with(INTERFACES_SETTINGS_GROUP)
        qs.setValue.assert_called_once_with(TEXTURE_PLANNER_PREFIX + "att_unit", "Wavelength")
        qs.endGroup.assert_called_once_with()


class TestTexturePlannerSettingsModel_GetSettingsDict(unittest.TestCase):
    def test_returns_value_from_get_setting_for_each_key(self):
        model = TexturePlannerSettingsModel()
        canned = {name: (True if t is bool else t(1) if t in (int, float) else "x") for name, t in SETTINGS_DICT.items()}

        with patch.object(TexturePlannerSettingsModel, "_get_setting", side_effect=lambda name, t, default: canned[name]):
            result = model.get_settings_dict()

        self.assertEqual(result, canned)

    def test_passes_type_and_default_for_each_key_to_get_setting(self):
        # get_settings_dict delegates the missing-value fallback to _get_setting by handing it the
        # per-key default; the fallback behaviour itself is covered in TestTexturePlannerSettingsModel_GetSetting.
        model = TexturePlannerSettingsModel()

        with patch.object(TexturePlannerSettingsModel, "_get_setting", return_value="x") as mock_get:
            model.get_settings_dict()

        self.assertEqual(mock_get.call_count, len(SETTINGS_DICT))
        for name, return_type in SETTINGS_DICT.items():
            mock_get.assert_any_call(name, return_type, DEFAULT_SETTINGS[name])


class TestTexturePlannerSettingsModel_SetSettingsDict(unittest.TestCase):
    def test_writes_each_known_setting_via_set_setting(self):
        model = TexturePlannerSettingsModel()
        payload = {"directions": False, "att_unit": "Wavelength", "att_point": 2.0}

        with patch.object(TexturePlannerSettingsModel, "_set_setting") as mock_set:
            model.set_settings_dict(payload)

        self.assertEqual(
            sorted(mock_set.call_args_list),
            sorted(
                [
                    call("directions", False),
                    call("att_unit", "Wavelength"),
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


class TestTexturePlannerSettingsModel_RoundTrip(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        QCoreApplication.setApplicationName("test_texture_planner")
        QCoreApplication.setOrganizationName("test_texture_planner_org")
        cls.settings_dir = tempfile.TemporaryDirectory()

    @classmethod
    def tearDownClass(cls):
        QSettings().clear()
        cls.settings_dir.cleanup()

    def setUp(self):
        QSettings().clear()
        self.model = TexturePlannerSettingsModel()

    def _assert_round_trips(self, settings):
        self.model.set_settings_dict(settings)

        result = self.model.get_settings_dict()

        self.assertEqual(result, settings)
        for name, value in settings.items():
            self.assertIsInstance(result[name], type(value), msg=name)

    def test_defaults_round_trip_unchanged(self):
        self._assert_round_trips(dict(DEFAULT_SETTINGS))

    def test_non_default_values_round_trip_unchanged(self):
        # flip bools from default
        settings = {name: (not value if isinstance(value, bool) else value) for name, value in DEFAULT_SETTINGS.items()}
        # change the rest of the values manually
        settings.update(
            {
                "stl_scale": "mm",
                "stl_x_degrees": 90.0,
                "stl_translation_vector": "1,2,3",
                "orientation_axes": "ZXZ",
                "mc_events_per_point": 500,
                "mc_simulate_in": "SampleAndEnvironment",
                "att_point": 2.75,
                "att_unit": "Wavelength",
            }
        )

        self._assert_round_trips(settings)

    def test_partial_write_leaves_other_settings_at_their_defaults(self):
        self.model.set_settings_dict({"att_unit": "Wavelength", "scattered": True})

        result = self.model.get_settings_dict()

        self.assertEqual(result["att_unit"], "Wavelength")
        self.assertIs(result["scattered"], True)
        for name in ("directions", "att_point", "mc_events_per_point", "stl_scale"):
            self.assertEqual(result[name], DEFAULT_SETTINGS[name], msg=name)


class TestTexturePlannerSettingsModel_DefaultsAlignment(unittest.TestCase):
    """DEFAULT_SETTINGS and SETTINGS_DICT must cover the same keys: a typed entry without a
    default would crash get_settings_dict, and a default without a type entry would never be
    read or written."""

    def test_default_settings_keys_match_settings_dict_keys(self):
        self.assertEqual(set(DEFAULT_SETTINGS.keys()), set(SETTINGS_DICT.keys()))


if __name__ == "__main__":
    unittest.main()
