# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import MagicMock, patch

from qtpy.QtCore import QLockFile, QSettings

from mantidqt.utils.qt.qsettings_change_aware import QSettingsChangeAware


class QSettingsChangeAwareTest(unittest.TestCase):
    def setUp(self):
        self._directory = TemporaryDirectory()
        self._filename = str(Path(self._directory.name) / "settings.ini")

    def tearDown(self):
        self._directory.cleanup()

    @patch("mantidqt.utils.qt.qsettings_change_aware.QSettings")
    def test_default_constructor_owns_a_qsettings_instance(self, qsettings_type):
        settings = MagicMock()
        settings.contains.return_value = False
        qsettings_type.return_value = settings

        writer = QSettingsChangeAware()

        self.assertTrue(writer.setValue("answer", 42))
        qsettings_type.assert_called_once_with()
        settings.setValue.assert_called_once_with("answer", 42)

    @patch("mantidqt.utils.qt.qsettings_change_aware.QSettings")
    def test_none_constructor_argument_owns_a_qsettings_instance(self, qsettings_type):
        qsettings_type.return_value = MagicMock()

        QSettingsChangeAware(None)

        qsettings_type.assert_called_once_with()

    def test_set_value_skips_equal_value_after_ini_type_conversion(self):
        self._seed("answer", 42)
        contents_before = Path(self._filename).read_bytes()
        settings = QSettings(self._filename, QSettings.IniFormat)
        writer = QSettingsChangeAware(settings)

        self.assertFalse(writer.setValue("answer", 42))

        self.assertFalse(writer.changed)
        self.assertEqual(contents_before, Path(self._filename).read_bytes())

    def test_set_value_writes_absent_and_changed_values(self):
        self._seed("existing", "old")
        settings = QSettings(self._filename, QSettings.IniFormat)
        writer = QSettingsChangeAware(settings)

        self.assertTrue(writer.setValue("missing", "new"))
        self.assertTrue(writer.setValue("existing", "new"))

        self.assertTrue(writer.changed)
        self.assertEqual("new", settings.value("missing"))
        self.assertEqual("new", settings.value("existing"))

    def test_custom_normalizer_can_define_semantic_equality(self):
        self._seed("path", "/data/")
        settings = QSettings(self._filename, QSettings.IniFormat)
        writer = QSettingsChangeAware(settings)

        changed = writer.setValue("path", "/data", normalizer=lambda value: str(value).rstrip("/"))

        self.assertFalse(changed)
        self.assertFalse(writer.changed)

    def test_remove_skips_missing_key(self):
        self._seed("preserved", True)
        contents_before = Path(self._filename).read_bytes()
        settings = QSettings(self._filename, QSettings.IniFormat)
        writer = QSettingsChangeAware(settings)

        self.assertFalse(writer.remove("missing"))

        self.assertFalse(writer.changed)
        self.assertEqual(contents_before, Path(self._filename).read_bytes())

    def test_remove_removes_exact_key(self):
        settings = QSettings(self._filename, QSettings.IniFormat)
        settings.setValue("remove", "value")
        settings.setValue("preserved", True)
        writer = QSettingsChangeAware(settings)

        self.assertTrue(writer.remove("remove"))

        self.assertFalse(settings.contains("remove"))
        self.assertTrue(settings.contains("preserved"))

    def test_remove_detects_group_with_only_descendant_keys(self):
        settings = QSettings(self._filename, QSettings.IniFormat)
        settings.setValue("group/child", "value")
        settings.setValue("preserved", True)
        writer = QSettingsChangeAware(settings)

        self.assertTrue(writer.remove("group"))

        self.assertTrue(writer.changed)
        self.assertFalse(settings.contains("group/child"))
        self.assertTrue(settings.contains("preserved"))

    def test_remove_with_empty_key_removes_current_group_only(self):
        settings = QSettings(self._filename, QSettings.IniFormat)
        settings.setValue("group/child", "value")
        settings.setValue("preserved", True)
        settings.beginGroup("group")
        writer = QSettingsChangeAware(settings)

        self.assertTrue(writer.remove(""))

        settings.endGroup()
        self.assertFalse(settings.contains("group/child"))
        self.assertTrue(settings.contains("preserved"))

    def test_unchanged_operations_complete_while_qsettings_lock_is_held(self):
        self._seed("answer", 42)
        lock = QLockFile(f"{self._filename}.lock")
        self.assertTrue(lock.tryLock(0))

        settings = QSettings(self._filename, QSettings.IniFormat)
        writer = QSettingsChangeAware(settings)
        self.assertFalse(writer.setValue("answer", 42))
        self.assertFalse(writer.remove("missing"))
        self.assertFalse(writer.changed)
        del writer
        del settings

        self.assertTrue(lock.isLocked())

    def _seed(self, key, value):
        settings = QSettings(self._filename, QSettings.IniFormat)
        settings.setValue(key, value)
        settings.sync()


if __name__ == "__main__":
    unittest.main()
