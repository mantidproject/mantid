# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock

from Engineering.gui.engineering_diffraction.settings import settings_model, settings_view, settings_presenter

dir_path = "Engineering.gui.engineering_diffraction.settings."


class SettingsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(settings_model.SettingsModel)
        self.view = mock.create_autospec(settings_view.SettingsView)
        self.presenter = settings_presenter.SettingsPresenter(self.model, self.view)
        self.presenter.settings = {}

    def test_load_existing_settings(self):
        self.model.get_settings_dict.return_value = {
            "save_location": "result",
            "full_calibration": "value",
            "recalc_vanadium": False
        }

        self.presenter.load_existing_settings()

        self.assertEqual(self.presenter.settings, {
            "full_calibration": "value",
            "save_location": "result",
            "recalc_vanadium": False
        })
        self.assertEqual(self.view.set_save_location.call_count, 1)
        self.assertEqual(self.view.set_full_calibration.call_count, 1)
        self.assertEqual(self.view.set_van_recalc.call_count, 1)

    def test_file_searched_on_opening(self):
        self.model.get_settings_dict.return_value = {
            "save_location": "result",
            "full_calibration": "value",
            "recalc_vanadium": False
        }

        self.presenter.load_existing_settings()

        self.assertEqual(1, self.view.find_full_calibration.call_count)
        self.assertEqual(1, self.view.find_save.call_count)

    def test_load_invalid_settings(self):
        self.model.get_settings_dict.return_value = {
            "foo": "dud",
            "bar": "result"
        }
        self.presenter.load_existing_settings()

        self.view.set_save_location.assert_called_with(settings_presenter.DEFAULT_SETTINGS["save_location"])
        self.view.set_full_calibration.assert_called_with(settings_presenter.DEFAULT_SETTINGS["full_calibration"])
        self.view.set_van_recalc.assert_called_with(settings_presenter.DEFAULT_SETTINGS["recalc_vanadium"])

    def test_save_new_settings(self):
        self.view.get_save_location.return_value = "save"
        self.view.get_full_calibration.return_value = "cal"
        self.view.get_van_recalc.return_value = False

        self.presenter.save_new_settings()

        self.assertEqual(self.presenter.settings, {
            "full_calibration": "cal",
            "save_location": "save",
            "recalc_vanadium": False
        })
        self.model.set_settings_dict.assert_called_with({
            "full_calibration": "cal",
            "save_location": "save",
            "recalc_vanadium": False
        })
        self.assertEqual(self.view.close.call_count, 0)

    def test_save_settings_and_close(self):
        self.view.get_save_location.return_value = "save"
        self.view.get_full_calibration.return_value = "cal"
        self.view.get_van_recalc.return_value = False

        self.presenter.save_and_close_dialog()

        self.assertEqual(self.presenter.settings, {
            "full_calibration": "cal",
            "save_location": "save",
            "recalc_vanadium": False
        })
        self.model.set_settings_dict.assert_called_with({
            "full_calibration": "cal",
            "save_location": "save",
            "recalc_vanadium": False
        })
        self.assertEqual(self.view.close.call_count, 1)


if __name__ == '__main__':
    unittest.main()
