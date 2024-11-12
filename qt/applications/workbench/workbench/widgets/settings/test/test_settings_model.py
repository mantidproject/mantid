# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest import TestCase
from unittest.mock import patch, MagicMock, call, mock_open

from workbench.widgets.settings.model import SettingsModel
from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel

test_config_settings = {
    "default.facility": "ISIS",
    "default.instrument": "INTER",
    "projectRecovery.enabled": False,
    "Notifications.Enabled": "On",
}

test_CONF_settings = {
    "project/prompt_on_deleting_workspace": True,
    "project/prompt_save_editor_modified": True,
    "MainWindow/font": "MS Shell Dlg 2,8,Regular",
}


class MockConfigService(object):
    def __init__(self):
        self.setString = MagicMock()
        self.getString = MagicMock(side_effect=lambda x: test_config_settings[x])
        self.hasProperty = MagicMock(side_effect=lambda x: x in test_config_settings)


class MockCONF(object):
    def __init__(self):
        self.set = MagicMock()
        self.get = MagicMock(side_effect=lambda x, type: test_CONF_settings[x])
        self.has = MagicMock(side_effect=lambda x: x in test_CONF_settings)


class MockConfigModel(ConfigSettingsChangesModel):
    def __init__(self):
        super().__init__()
        self.apply_changes = MagicMock()
        self.get_changes = MagicMock(return_value={})
        self.properties_to_be_changed = MagicMock(return_value=[])


class SettingsModelTest(TestCase):
    def setUp(self) -> None:
        self.mock_settings_category_model_1 = MockConfigModel()
        self.mock_settings_category_model_2 = MockConfigModel()
        self.model = SettingsModel([self.mock_settings_category_model_1, self.mock_settings_category_model_2])

    def test_register_property_which_needs_a_restart(self):
        self.assertEqual(self.model.properties_which_need_a_restart, [])

        self.model.register_property_which_needs_a_restart("my cool property")
        self.assertEqual(self.model.properties_which_need_a_restart, ["my cool property"])

        self.model.register_property_which_needs_a_restart("second property")
        self.assertEqual(self.model.properties_which_need_a_restart, ["my cool property", "second property"])

    def test_apply_all_settings(self):
        self.model.apply_all_settings()
        self.mock_settings_category_model_1.apply_changes.assert_called_once()
        self.mock_settings_category_model_2.apply_changes.assert_called_once()

    def test_unsaved_changes(self):
        self.assertEqual(self.model.unsaved_changes(), {})

        self.mock_settings_category_model_1.get_changes.return_value = {"font": "comic sans", "font size": "96"}
        self.mock_settings_category_model_2.get_changes.return_value = {"x_min": "1", "x scale": "Linear"}

        self.assertEqual(self.model.unsaved_changes(), {"font": "comic sans", "font size": "96", "x_min": "1", "x scale": "Linear"})

    def test_potential_changes_that_need_a_restart(self):
        self.assertEqual(self.model.potential_changes_that_need_a_restart(), [])

        self.model.properties_which_need_a_restart = ["my cool property", "second property"]
        self.assertEqual(self.model.potential_changes_that_need_a_restart(), [])

        self.mock_settings_category_model_1.properties_to_be_changed.return_value = ["second property"]
        self.assertEqual(self.model.potential_changes_that_need_a_restart(), ["second property"])

    @patch("workbench.widgets.settings.model.ConfigService", new_callable=MockConfigService)
    @patch("workbench.widgets.settings.model.CONF", new_callable=MockCONF)
    def test_save_settings_to_file(self, _, __):
        settings = list(test_config_settings.keys()) + list(test_CONF_settings.keys())
        expected_calls = [call(setting + "=" + str(test_config_settings[setting]) + "\n") for setting in test_config_settings] + [
            call(setting + "=" + str(test_CONF_settings[setting]) + "\n") for setting in test_CONF_settings
        ]

        with patch("workbench.widgets.settings.model.open", mock_open()) as open_mock:
            self.model.save_settings_to_file("filepath", settings)

        open_mock.assert_called_once_with("filepath", "w")
        open_mock().write.assert_has_calls(expected_calls)

    @patch("workbench.widgets.settings.model.ConfigService", new_callable=MockConfigService)
    @patch("workbench.widgets.settings.model.CONF", new_callable=MockCONF)
    def test_load_settings_from_file(self, mock_conf, mock_configService):
        settings = list(test_config_settings.keys()) + list(test_CONF_settings.keys())
        file_data = "\n".join(
            [setting + "=" + str(test_config_settings[setting]) for setting in test_config_settings]
            + [setting + "=" + str(test_CONF_settings[setting]) for setting in test_CONF_settings]
        )

        with patch("workbench.widgets.settings.model.open", mock_open(read_data=file_data)) as open_mock:
            self.model.load_settings_from_file("filepath", settings)

        open_mock.assert_called_once_with("filepath", "r")
        mock_configService.setString.assert_has_calls([call(prop, str(value)) for prop, value in test_config_settings.items()])
        mock_conf.set.assert_has_calls([call(prop, value) for prop, value in test_CONF_settings.items()])
