# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from os import path

SETTINGS_DICT = {"save_location": str, "full_calibration": str, "recalc_vanadium": bool}

DEFAULT_SETTINGS = {
    "full_calibration": "",
    "save_location": path.join(path.expanduser("~"), "Engineering_Mantid"),
    "recalc_vanadium": False
}


class SettingsPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.settings = {}

        # Connect view signals
        self.view.set_on_apply_clicked(self.save_new_settings)
        self.view.set_on_ok_clicked(self.save_and_close_dialog)
        self.view.set_on_cancel_clicked(self.close_dialog)

    def show(self):
        self.view.show()

    def load_existing_settings(self):
        self.load_settings_from_file_or_default()
        self._show_settings_in_view()

    def close_dialog(self):
        self.view.close()

    def save_and_close_dialog(self):
        self.save_new_settings()
        self.close_dialog()

    def save_new_settings(self):
        self._collect_new_settings_from_view()
        self._save_settings_to_file()

    def _collect_new_settings_from_view(self):
        self.settings["save_location"] = self.view.get_save_location()
        self.settings["full_calibration"] = self.view.get_full_calibration()
        self.settings["recalc_vanadium"] = self.view.get_van_recalc()

    def _show_settings_in_view(self):
        if self._validate_settings(self.settings):
            self.view.set_save_location(self.settings["save_location"])
            self.view.set_full_calibration(self.settings["full_calibration"])
            self.view.set_van_recalc(self.settings["recalc_vanadium"])
        self._find_files()

    def _find_files(self):
        self.view.find_full_calibration()
        self.view.find_save()

    def _save_settings_to_file(self):
        if self._validate_settings(self.settings):
            self.model.set_settings_dict(self.settings)

    def load_settings_from_file_or_default(self):
        self.settings = self.model.get_settings_dict(SETTINGS_DICT)
        if not self._validate_settings(self.settings):
            self.settings = DEFAULT_SETTINGS.copy()
            self._save_settings_to_file()

    @staticmethod
    def _validate_settings(settings):
        try:
            all_keys = settings.keys() == SETTINGS_DICT.keys()
            save_location = str(settings["save_location"])
            save_valid = save_location != "" and save_location is not None
            recalc_valid = settings["recalc_vanadium"] is not None
            return all_keys and save_valid and recalc_valid
        except KeyError:  # Settings contained invalid key.
            return False
