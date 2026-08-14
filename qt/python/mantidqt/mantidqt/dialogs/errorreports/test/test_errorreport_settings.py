# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from qtpy.QtCore import QSettings

from mantidqt.dialogs.errorreports import settings


class ErrorReportSettingsTest(unittest.TestCase):
    def test_organization_comes_from_workbench_config(self):
        from workbench.config import ORGANIZATION

        self.assertEqual(settings._workbench_organization(), ORGANIZATION)

    def test_legacy_application_comes_from_workbench_config(self):
        from workbench.config import APPNAME

        self.assertEqual(settings._workbench_application(), APPNAME)

    @mock.patch("mantidqt.dialogs.errorreports.settings.QSettings")
    def test_create_error_reporter_settings_uses_dedicated_file_identity(self, qsettings):
        qsettings.IniFormat = QSettings.IniFormat
        qsettings.UserScope = QSettings.UserScope

        with mock.patch.object(settings, "_workbench_organization", return_value="workbench-organization"):
            result = settings.create_error_reporter_settings()

        qsettings.assert_called_once_with(QSettings.IniFormat, QSettings.UserScope, "workbench-organization", "mantid-error-reporter")
        self.assertIs(result, qsettings.return_value)

    @mock.patch("mantidqt.dialogs.errorreports.settings.QSettings")
    def test_create_legacy_workbench_settings_uses_workbench_identity(self, qsettings):
        qsettings.IniFormat = QSettings.IniFormat
        qsettings.UserScope = QSettings.UserScope

        with (
            mock.patch.object(settings, "_workbench_organization", return_value="workbench-organization"),
            mock.patch.object(settings, "_workbench_application", return_value="workbench-application"),
        ):
            result = settings.create_legacy_workbench_settings()

        qsettings.assert_called_once_with(QSettings.IniFormat, QSettings.UserScope, "workbench-organization", "workbench-application")
        self.assertIs(result, qsettings.return_value)

    @mock.patch("mantidqt.dialogs.errorreports.settings.create_legacy_workbench_settings")
    @mock.patch("mantidqt.dialogs.errorreports.settings.create_error_reporter_settings")
    def test_read_contact_information_does_not_inspect_legacy_store_when_both_dedicated_values_are_set(
        self, create_reporter_settings, create_legacy_settings
    ):
        reporter_settings = create_reporter_settings.return_value
        reporter_settings.contains.return_value = True
        reporter_settings.value.side_effect = lambda key, default, type: {"Name": "", "Email": "reporter@example.com"}[key]

        result = settings.read_contact_information()

        self.assertEqual(result, ("", "reporter@example.com"))
        create_legacy_settings.assert_not_called()

    @mock.patch("mantidqt.dialogs.errorreports.settings.create_legacy_workbench_settings")
    @mock.patch("mantidqt.dialogs.errorreports.settings.create_error_reporter_settings")
    def test_read_contact_information_falls_back_only_for_missing_dedicated_values(self, create_reporter_settings, create_legacy_settings):
        reporter_settings = create_reporter_settings.return_value
        reporter_settings.contains.side_effect = lambda key: key == "Name"
        reporter_settings.value.return_value = "Reporter Name"
        legacy_settings = create_legacy_settings.return_value
        legacy_settings.value.return_value = "legacy@example.com"

        result = settings.read_contact_information()

        self.assertEqual(result, ("Reporter Name", "legacy@example.com"))
        reporter_settings.value.assert_called_once_with("Name", "", type=str)
        legacy_settings.value.assert_called_once_with("Email", "", type=str)
        for store in (reporter_settings, legacy_settings):
            store.setValue.assert_not_called()
            store.remove.assert_not_called()
            store.sync.assert_not_called()


if __name__ == "__main__":
    unittest.main()
