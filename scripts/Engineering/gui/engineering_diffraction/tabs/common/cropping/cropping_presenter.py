# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)


class CroppingPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

        self.bank = 0
        self.custom_spectra_enabled = False
        self.custom_spectra = ""
        self.custom_valid = True

        # Connect view signals to local functions
        self.view.set_on_combo_changed(self.on_combo_changed)
        self.view.set_on_custom_spectra_changed(self.on_spectra_changed)

        self.on_combo_changed(0)

    # Signal Activated Functions

    def on_combo_changed(self, index):
        if index == 0:
            self.bank = 1
            self.custom_spectra_enabled = False
            self.set_custom_visibility(False)
        elif index == 1:
            self.bank = 2
            self.custom_spectra_enabled = False
            self.set_custom_visibility(False)
        else:
            self.bank = 0
            self.custom_spectra_enabled = True
            self.set_custom_visibility(True)

    def on_spectra_changed(self, text):
        error, value = self.model.validate_and_clean_spectrum_numbers(text)
        if error == "":
            self.custom_spectra = value
        self.set_invalid_status(error)

    # Getters

    def get_custom_spectra(self):
        return self.custom_spectra

    def get_custom_spectra_enabled(self):
        return self.custom_spectra_enabled

    def get_bank(self):
        return self.bank

    def is_valid(self):
        return self.custom_valid

    # Setters

    def set_custom_visibility(self, visible):
        if visible:
            self.on_spectra_changed(self.view.get_custom_spectra_text())
            self.view.set_custom_spectra_entry_visible()
        else:
            self.custom_valid = True  # Make custom valid if not used, makes validation easier.
            self.view.set_custom_spectra_entry_hidden()

    def set_invalid_status(self, text):
        if text:
            self.view.set_invalid_indicator_visible(text)
            self.custom_valid = False
        else:
            self.view.set_invalid_indicator_hidden()
            self.custom_valid = True
