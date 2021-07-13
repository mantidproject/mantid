# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_view import CroppingView
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_model import CroppingModel


class CroppingPresenter(object):
    def __init__(self, parent, view=None, model=None):
        self.parent = parent
        self.model = model if model else CroppingModel()
        self.view = view if view else CroppingView(parent)

        self.bank = 0
        self.custom_spectra_enabled = False
        self.custom_spectra = None
        self.spectra_valid = True
        self.custom_calfile_enabled = True
        self.custom_calfile = None
        self.calfile_valid = True

        # Connect view signals to local functions
        self.view.set_on_combo_changed(self.on_combo_changed)
        self.view.set_on_custom_calfile_changed(self.on_calfile_changed)
        self.view.set_on_custom_spectra_changed(self.on_spectra_changed)

        self.on_combo_changed(0)

    # Signal Activated Functions

    def on_combo_changed(self, index):
        if index == 0:  # custom calfile
            self.bank = 0
            self.custom_calfile_enabled = True
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(True, False)
        elif index == 1:  # north
            self.bank = 1
            self.custom_calfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)
        elif index == 2:  # south
            self.bank = 2
            self.custom_calfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)
        else:  # cropped
            self.bank = 0
            self.custom_calfile_enabled = False
            self.custom_spectra_enabled = True
            self.set_custom_widgets_visibility(False, True)

    def on_calfile_changed(self):
        valid = self.view.finder_custom.isValid()
        self.calfile_valid = valid
        if valid:
            self.custom_calfile = self.view.get_custom_calfile()
        else:
            self.custom_calfile = None

    def on_spectra_changed(self, text):
        error, value = self.model.validate_and_clean_spectrum_numbers(text)
        if error == "":
            self.custom_spectra = value
        else:
            self.custom_spectra = None
        self.set_invalid_spectra_status(error)

    # Getters

    def get_custom_calfile(self):
        return self.custom_calfile

    def get_custom_calfile_enabled(self):
        return self.custom_calfile_enabled

    def get_custom_spectra(self):
        return self.custom_spectra

    def get_custom_spectra_enabled(self):
        return self.custom_spectra_enabled

    def get_bank(self):
        return self.bank

    def is_calfile_valid(self):
        return self.calfile_valid

    def is_spectra_valid(self):
        return self.spectra_valid

    # Setters

    def set_custom_widgets_visibility(self, custom_visible: bool, crop_visible: bool) -> None:
        if not (custom_visible or crop_visible):
            self.view.set_custom_calfile_widget_hidden()
            self.view.set_custom_spectra_widget_hidden()
            self.calfile_valid = True  # Make custom valid if not used, makes validation easier.
            self.spectra_valid = True
        elif custom_visible:
            self.view.set_custom_spectra_widget_hidden()
            self.spectra_valid = True
            self.on_calfile_changed()
            self.view.set_custom_calfile_widget_visible()
        elif crop_visible:
            self.view.set_custom_calfile_widget_hidden()
            self.calfile_valid = True
            self.on_spectra_changed(self.view.get_custom_spectra_text())
            self.view.set_custom_spectra_widget_visible()

    def set_invalid_spectra_status(self, text):
        if text:
            self.view.set_crop_invalid_indicator_visible(text)
            self.spectra_valid = False
        else:
            self.view.set_crop_invalid_indicator_hidden()
            self.spectra_valid = True
