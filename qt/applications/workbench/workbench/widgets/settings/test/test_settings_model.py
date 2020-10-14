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

test_config_settings = {"default.facility": "ISIS",
                        "default.instrument": "INTER",
                        "projectRecovery.enabled": False,
                        'Notifications.Enabled': "On"}

test_CONF_settings = {'project/prompt_on_deleting_workspace': True,
                      'project/prompt_save_editor_modified': True,
                      'MainWindow/font' : "MS Shell Dlg 2,8,Regular"}


class MockConfigService(object):

    def __init__(self):
        self.setString = MagicMock()
        self.getString = MagicMock(side_effect=lambda x: test_config_settings[x])
        self.hasProperty = MagicMock(side_effect=lambda x: x in test_config_settings)


class MockCONF(object):

    def __init__(self):
        self.set = MagicMock()
        self.get = MagicMock(side_effect=lambda x: test_CONF_settings[x])
        self.has = MagicMock(side_effect=lambda x: x in test_CONF_settings)


class SettingsModelTest(TestCase):

    @patch("workbench.widgets.settings.model.ConfigService", new_callable=MockConfigService)
    @patch("workbench.widgets.settings.model.CONF", new_callable=MockCONF)
    def test_save_settings_to_file(self, mock_conf, mock_configService):
        settings = list(test_config_settings.keys()) + list(test_CONF_settings.keys())
        expected_calls = [call(setting + '=' + str(test_config_settings[setting]) + '\n') for setting in test_config_settings] + \
                         [call(setting + '=' + str(test_CONF_settings[setting]) + '\n') for setting in test_CONF_settings]

        with patch("workbench.widgets.settings.model.open", mock_open()) as open_mock:
            SettingsModel.save_settings_to_file("filepath", settings)

        open_mock.assert_called_once_with("filepath", 'w')
        open_mock().write.assert_has_calls(expected_calls)

    @patch("workbench.widgets.settings.model.ConfigService", new_callable=MockConfigService)
    @patch("workbench.widgets.settings.model.CONF", new_callable=MockCONF)
    def test_load_settings_from_file(self, mock_conf, mock_configService):
        settings = list(test_config_settings.keys()) + list(test_CONF_settings.keys())
        file_data = '\n'.join([setting + '=' + str(test_config_settings[setting]) for setting in test_config_settings]
                              + [setting + '=' + str(test_CONF_settings[setting]) for setting in test_CONF_settings])

        with patch("workbench.widgets.settings.model.open", mock_open(read_data=file_data)) as open_mock:
            SettingsModel.load_settings_from_file("filepath", settings)

        open_mock.assert_called_once_with("filepath", 'r')
        mock_configService.setString.assert_has_calls([call(prop, str(value)) for prop, value in test_config_settings.items()])
        mock_conf.set.assert_has_calls([call(prop, value) for prop, value in test_CONF_settings.items()])
