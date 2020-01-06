# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.settings.settings_helper import set_setting, get_setting

from qtpy.QtCore import QSettings, QCoreApplication

from shutil import rmtree
from os.path import dirname

import unittest

GROUP = "CustomInterfaces"
PREFIX = "EngineeringDiffraction2/"


class SettingsHelperTest(unittest.TestCase):
    def tearDown(self):
        settings = QSettings()
        settings.clear()
        rmtree(dirname(settings.fileName()))

    def setUp(self):
        settings = QSettings()
        settings.clear()

    @classmethod
    def setUpClass(cls):
        QCoreApplication.setApplicationName("test1")
        QCoreApplication.setOrganizationName("org1")
        QSettings.setDefaultFormat(QSettings.IniFormat)

    def test_set_setting_with_string(self):
        set_setting(GROUP, PREFIX, "something", "value")

        settings = QSettings()
        settings.beginGroup(GROUP)
        returned = settings.value(PREFIX + "something")
        settings.endGroup()
        self.assertEqual(returned, "value")

    def test_set_setting_with_bool_false(self):
        set_setting(GROUP, PREFIX, "something", False)

        settings = QSettings()
        settings.beginGroup("CustomInterfaces")
        returned = settings.value("EngineeringDiffraction2/" + "something", type=bool)
        settings.endGroup()
        self.assertEqual(returned, False)

    def test_set_setting_with_bool_true(self):
        set_setting(GROUP, PREFIX, "something", True)

        settings = QSettings()
        settings.beginGroup("CustomInterfaces")
        returned = settings.value("EngineeringDiffraction2/" + "something", type=bool)
        settings.endGroup()
        self.assertEqual(returned, True)

    def test_set_setting_with_int(self):
        set_setting(GROUP, PREFIX, "something", 10)

        settings = QSettings()
        settings.beginGroup(GROUP)
        returned = settings.value(PREFIX + "something", type=int)
        settings.endGroup()
        self.assertEqual(returned, 10)

    def test_get_setting_with_string(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", "value")
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something"), "value")

    def test_get_setting_with_bool_false(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", False)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something", return_type=bool), False)

    def test_get_setting_with_bool_true(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", True)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something", return_type=bool), True)

    def test_get_setting_with_int(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", 10)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something", return_type=int), 10)

    def test_get_setting_with_invalid(self):
        self.assertEqual(get_setting(GROUP, PREFIX, "something"), "")


if __name__ == '__main__':
    unittest.main()
