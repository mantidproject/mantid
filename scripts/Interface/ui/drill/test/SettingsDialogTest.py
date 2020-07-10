# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication

from Interface.ui.drill.view.SettingsDialog import SettingsDialog


app = QApplication(sys.argv)


class SettingsDialogTest(unittest.TestCase):

    def setUp(self):
        patch = mock.patch('Interface.ui.drill.view.SettingsDialog.Setting')
        self.mSetting = patch.start()
        self.addCleanup(patch.stop)

        self.dialog = SettingsDialog()
        self.dialog.formLayout = mock.Mock()

    def test_init(self):
        self.assertEqual(self.dialog.settings, {})

    def test_initWidgets(self):
        self.dialog.initWidgets({}, {}, {})
        self.assertEqual(self.dialog.settings, {})
        self.dialog.initWidgets({"name": "type"}, {"name": []}, {"name": "doc"})
        self.mSetting.assert_called_once_with("name", [], "type", "doc")
        self.assertDictEqual(self.dialog.settings,
                             {"name": self.mSetting.return_value})
        self.mSetting.return_value.valueChanged.connect.assert_called_once()
        self.dialog.formLayout.addRow.assert_called_once()

    def test_setSettings(self):
        self.dialog.setSettings({})
        self.assertDictEqual(self.dialog.settings, {})

        self.dialog.settings["name1"] = mock.Mock()
        self.dialog.setSettings({"name2": "v2"})
        self.dialog.settings["name1"].setter.assert_not_called()

        self.dialog.setSettings({"name1": "v2"})
        self.dialog.settings["name1"].setter.assert_called_once_with("v2")

    def test_getSettings(self):
        self.dialog.settings["name1"] = mock.Mock()
        self.dialog.settings["name1"].getter.return_value = "v1"
        self.dialog.settings["name2"] = mock.Mock()
        self.dialog.settings["name2"].getter.return_value = True
        self.dialog.settings["name3"] = mock.Mock()
        self.dialog.settings["name3"].getter.return_value = 15.2

        d = self.dialog.getSettings()
        self.assertDictEqual(d, {"name1": "v1", "name2": True, "name3": 15.2})

    def test_getSettingValue(self):
        self.dialog.settings["name1"] = mock.Mock()
        self.dialog.settings["name1"].getter.return_value = "v1"

        v = self.dialog.getSettingValue("name1")
        self.assertEqual(v, "v1")

        v = self.dialog.getSettingValue("name2")
        self.assertEqual(v, "")

    def test_onSettingValidation(self):
        self.dialog.onSettingValidation("name", False)

        mS = mock.Mock()
        self.dialog.settings["name"] = mS
        self.dialog.onSettingValidation("name", False, "msg")
        mS.widget.setStyleSheet.assert_called_once()
        mS.widget.setToolTip.assert_called_once_with("msg")
        mS.reset_mock()
        self.dialog.onSettingValidation("name", False)
        mS.widget.setStyleSheet.assert_called_once()
        mS.widget.setToolTip.assert_called_once()
        mS.reset_mock()
        self.dialog.onSettingValidation("name", True, "msg")
        mS.widget.setStyleSheet.assert_called_once()
        mS.widget.setToolTip.assert_called_once()


if __name__ == "__main__":
    unittest.main()
