# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import set_setting, get_setting
from qtpy.QtCore import QSettings, QCoreApplication

from shutil import rmtree
from os.path import dirname

import tempfile
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
        cls.settings_dir = tempfile.TemporaryDirectory()
        QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, cls.settings_dir.name)

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

    def test_get_setting_int_without_specifying_type(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", 10)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something"), "10")

    def test_get_setting_bool_without_specifying_type(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", True)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something"), "true")

    def test_get_setting_bool_specifying_int(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", True)
        settings.endGroup()

        self.assertEqual(get_setting(GROUP, PREFIX, "something", return_type=int), 1)

    def test_get_setting_int_specifying_bool(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", 10)
        settings.endGroup()

        self.assertRaises(TypeError, get_setting, GROUP, PREFIX, "something", return_type=bool)

    def test_get_setting_string_specifying_int(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", "some setting")
        settings.endGroup()

        self.assertRaises(TypeError, get_setting, GROUP, PREFIX, "something", return_type=int)

    def test_get_setting_string_specifying_bool(self):
        settings = QSettings()
        settings.beginGroup(GROUP)
        settings.setValue(PREFIX + "something", "a")
        settings.endGroup()

        self.assertRaises(TypeError, get_setting, GROUP, PREFIX, "something", return_type=bool)

    # ---------------- RB scoping ----------------
    # "rd_dir" is in SCOPED_SETTINGS; "save_location" is not.

    def test_rb_scoped_setting_is_written_under_the_rb(self):
        set_setting(GROUP, PREFIX, "rd_dir", "9,9,9", rb="12345")

        settings = QSettings()
        settings.beginGroup(GROUP)
        scoped = settings.value(PREFIX + "rb/12345/rd_dir")
        unscoped = settings.value(PREFIX + "rd_dir")
        settings.endGroup()
        self.assertEqual(scoped, "9,9,9")
        # the global value must be left alone
        self.assertIsNone(unscoped)

    def test_unscoped_setting_ignores_the_rb(self):
        set_setting(GROUP, PREFIX, "save_location", "/some/path", rb="12345")

        self.assertEqual(get_setting(GROUP, PREFIX, "save_location"), "/some/path")
        self.assertEqual(get_setting(GROUP, PREFIX, "save_location", rb="12345"), "/some/path")

    def test_scoped_setting_inherits_global_until_the_rb_has_its_own(self):
        set_setting(GROUP, PREFIX, "rd_dir", "0,0,1")

        # an RB that has never been saved reads the user's global value, not a hard-coded default
        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="12345"), "0,0,1")

        set_setting(GROUP, PREFIX, "rd_dir", "1,0,0", rb="12345")

        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="12345"), "1,0,0")
        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir"), "0,0,1")
        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="99999"), "0,0,1")

    def test_scopes_are_isolated_from_each_other(self):
        set_setting(GROUP, PREFIX, "rd_dir", "1,0,0", rb="A")
        set_setting(GROUP, PREFIX, "rd_dir", "0,1,0", rb="B")

        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="A"), "1,0,0")
        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="B"), "0,1,0")

    def test_blank_rb_is_the_global_scope(self):
        set_setting(GROUP, PREFIX, "rd_dir", "0,0,1")

        for blank in (None, "", "   "):
            self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb=blank), "0,0,1")

    def test_rb_separators_are_sanitised_into_one_scope(self):
        # "/" and "\" are permitted in an RB but are QSettings group separators; both map to "$"
        set_setting(GROUP, PREFIX, "rd_dir", "1,2,3", rb="2024/1")

        settings = QSettings()
        settings.beginGroup(GROUP)
        stored = settings.value(PREFIX + "rb/2024$1/rd_dir")
        settings.endGroup()
        self.assertEqual(stored, "1,2,3")
        # the same experiment folder however it was typed
        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="2024\\1"), "1,2,3")

    def test_surrounding_whitespace_in_rb_is_ignored(self):
        set_setting(GROUP, PREFIX, "rd_dir", "4,5,6", rb="12345")

        self.assertEqual(get_setting(GROUP, PREFIX, "rd_dir", rb="  12345  "), "4,5,6")


if __name__ == "__main__":
    unittest.main()
