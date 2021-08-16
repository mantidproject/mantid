# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import unittest
from os import path

from unittest import mock
from Engineering.gui.engineering_diffraction.settings import settings_model, settings_view, settings_presenter


class SettingsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(settings_model.SettingsModel)
        self.view = mock.create_autospec(settings_view.SettingsView)
        self.presenter = settings_presenter.SettingsPresenter(self.model, self.view)
        self.presenter.settings = {}
        self.settings = {"save_location": "save",
                         "full_calibration": "cal",
                         "recalc_vanadium": False,
                         "logs": "some,logs",
                         "primary_log": "some",
                         "sort_ascending": True,
                         "default_peak": "BackToBackExponential"
                         }

    def test_load_existing_settings(self):
        self.model.get_settings_dict.return_value = self.settings.copy()

        self.presenter.load_settings_from_file_or_default()

        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_not_called()

    def test_file_searched_on_opening(self):
        self.model.get_settings_dict.return_value = self.settings.copy()

        self.presenter.load_settings_from_file_or_default()

        self.assertEqual(1, self.view.find_full_calibration.call_count)
        self.assertEqual(1, self.view.find_save.call_count)

    def test_load_invalid_settings(self):

        def return_value(self):
            return {"foo": "dud", "bar": "result"}

        self.model.get_settings_dict.side_effect = return_value
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.load_settings_from_file_or_default()

        self.assertEqual(self.presenter.settings, settings_presenter.DEFAULT_SETTINGS)
        self.model.set_settings_dict.assert_called_once()  # called to replace invalid settings

    def test_load_invalid_settings_correct_keys(self):
        def return_value(self):
            return {"save_location": "save",
                    "full_calibration": "", # invalid
                    "recalc_vanadium": True, # not the default but valid
                    "logs": "some,logs",
                    "primary_log": "some",
                    "sort_ascending": True,
                    "default_peak": "BackToBackExponential"
                    }

        self.model.get_settings_dict.side_effect = return_value
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.load_settings_from_file_or_default()

        expected_dict = return_value(self)
        expected_dict["full_calibration"] = settings_presenter.DEFAULT_SETTINGS["full_calibration"]
        self.assertEqual(self.presenter.settings, expected_dict)
        self.model.set_settings_dict.assert_called_once()  # called to replace invalid settings

    def test_save_new_settings(self):
        self.view.get_save_location.return_value = self.settings['save_location'][:]
        self.view.get_full_calibration.return_value = self.settings['full_calibration'][:]
        self.view.get_van_recalc.return_value = self.settings['recalc_vanadium']
        self.view.get_checked_logs.return_value = self.settings['logs'][:]
        self.view.get_primary_log.return_value = self.settings['primary_log'][:]
        self.view.get_ascending_checked.return_value = self.settings['sort_ascending']
        self.view.get_peak_function.return_value = self.settings["default_peak"]
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.save_new_settings()

        self.assertEqual(self.view.close.call_count, 0)
        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_called_with(self.settings)
        self.assertEqual(self.presenter.savedir_notifier.notify_subscribers.call_count, 1)

    def test_show(self):
        self.presenter.settings = self.settings.copy()

        self.presenter.show()

        # check that view is updated before being shown
        self.view.set_save_location.assert_called_with(self.settings["save_location"])
        self.view.set_full_calibration.assert_called_with(self.settings["full_calibration"])
        self.view.set_van_recalc.assert_called_with(self.settings["recalc_vanadium"])
        self.view.set_checked_logs.assert_called_with(self.settings["logs"])
        self.view.set_primary_log_combobox.assert_called_with(self.settings["primary_log"])
        self.view.set_ascending_checked.assert_called_with(self.settings["sort_ascending"])
        self.view.set_peak_function.assert_called_with(self.settings["default_peak"])

    def test_save_settings_and_close(self):
        self.view.get_save_location.return_value = self.settings['save_location'][:]
        self.view.get_full_calibration.return_value = self.settings['full_calibration'][:]
        self.view.get_van_recalc.return_value = self.settings['recalc_vanadium']
        self.view.get_checked_logs.return_value = self.settings['logs'][:]
        self.view.get_primary_log.return_value = self.settings['primary_log'][:]
        self.view.get_ascending_checked.return_value = self.settings['sort_ascending']
        self.view.get_peak_function.return_value = self.settings["default_peak"]
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.save_and_close_dialog()

        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_called_with(self.settings)
        self.assertEqual(self.view.close.call_count, 1)
        self.assertEqual(self.presenter.savedir_notifier.notify_subscribers.call_count, 1)

    def test_settings_not_changed_when_cancelled(self):
        self.presenter.close_dialog()
        self.model.set_settings_dict.assert_not_called()

    def test_default_calib_file_correct_location(self):
        self.assertTrue(path.exists(settings_presenter.DEFAULT_SETTINGS["full_calibration"]))


if __name__ == '__main__':
    unittest.main()
