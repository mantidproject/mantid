# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.settings.settings_model import SettingsModel

dir_path = "Engineering.gui.engineering_diffraction.settings."


class SettingsModelTest(unittest.TestCase):
    def setUp(self):
        self.model = SettingsModel()

    @patch(dir_path + "settings_model.set_setting")
    def test_set_setting(self, set_setting_mock):
        self.model.set_setting("name", "value")
        set_setting_mock.assert_called_with("CustomInterfaces", "EngineeringDiffraction2/", "name", "value")

    @patch(dir_path + "settings_model.get_setting")
    def test_get_setting(self, get_setting_mock):
        self.model.get_setting("name")
        get_setting_mock.assert_called_with("CustomInterfaces", "EngineeringDiffraction2/", "name", return_type=str)

    @patch(dir_path + "settings_model.set_setting")
    def test_set_settings_dict(self, set_setting_mock):
        self.model.set_settings_dict({"name": "value", "namebool": False, "namenum": 10})
        self.assertEqual(set_setting_mock.call_count, 3)
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name", "value")
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "namebool", False)
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "namenum", 10)

    @patch(dir_path + "settings_model.get_setting")
    def test_get_settings_dict(self, get_setting_mock):
        get_setting_mock.return_value = "value"
        self.assertEqual(self.model.get_settings_dict({"name1": str, "name2": str}),
                         {'name1': 'value', 'name2': 'value'})
        self.assertEqual(get_setting_mock.call_count, 2)
        get_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name1", return_type=str)
        get_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name2", return_type=str)


if __name__ == '__main__':
    unittest.main()
