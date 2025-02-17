# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_view import CroppingView
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_model import CroppingModel
from Engineering.EnggUtils import GROUP


class CroppingPresenter(object):
    def __init__(self, parent, view=None, model=None):
        self.parent = parent
        self.model = model if model else CroppingModel()
        self.view = view if view else CroppingView(parent)

        self.group = None  # default if no cropping requested
        self.custom_spectra_enabled = False
        self.custom_spectra = None
        self.spectra_valid = True
        self.custom_groupingfile_enabled = True
        self.custom_groupingfile = None
        self.groupingfile_valid = True

        # Connect view signals to local functions
        self.view.set_on_combo_changed(self.on_combo_changed)
        self.view.set_on_custom_groupingfile_changed(self.on_groupingfile_changed)
        self.view.set_on_custom_spectra_changed(self.on_spectra_changed)

        self.on_combo_changed(0)

    # Signal Activated Functions

    def on_combo_changed(self, index):
        if index == 0:  # custom grouping file
            self.group = GROUP.CUSTOM
            self.custom_groupingfile_enabled = True
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(True, False)
        elif index == 1:  # north
            self.group = GROUP.NORTH
            self.custom_groupingfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)
        elif index == 2:  # south
            self.group = GROUP.SOUTH
            self.custom_groupingfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)
        elif index == 3:  # cropped
            self.group = GROUP.CROPPED
            self.custom_groupingfile_enabled = False
            self.custom_spectra_enabled = True
            self.set_custom_widgets_visibility(False, True)
        elif index == 4:  # texture 20 grouping
            self.group = GROUP.TEXTURE20
            self.custom_groupingfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)
        else:
            self.group = GROUP.TEXTURE30
            self.custom_groupingfile_enabled = False
            self.custom_spectra_enabled = False
            self.set_custom_widgets_visibility(False, False)

    def on_groupingfile_changed(self):
        valid = self.view.finder_custom.isValid()
        self.groupingfile_valid = valid
        if valid:
            self.custom_groupingfile = self.view.get_custom_groupingfile()
        else:
            self.custom_groupingfile = None

    def on_spectra_changed(self, text):
        error, value = self.model.validate_and_clean_spectrum_numbers(text)
        if error == "":
            self.custom_spectra = value
        else:
            self.custom_spectra = None
        self.set_invalid_spectra_status(error)

    # Getters

    def get_custom_groupingfile(self):
        return self.custom_groupingfile

    def get_custom_groupingfile_enabled(self):
        return self.custom_groupingfile_enabled

    def get_custom_spectra(self):
        return self.custom_spectra

    def get_custom_spectra_enabled(self):
        return self.custom_spectra_enabled

    def get_group(self):
        return self.group

    def is_groupingfile_valid(self):
        return self.groupingfile_valid

    def is_spectra_valid(self):
        return self.spectra_valid

    # Setters

    def set_custom_widgets_visibility(self, custom_visible: bool, crop_visible: bool) -> None:
        if not (custom_visible or crop_visible):
            self.view.set_custom_groupingfile_widget_hidden()
            self.view.set_custom_spectra_widget_hidden()
            self.groupingfile_valid = True  # Make custom valid if not used, makes validation easier.
            self.spectra_valid = True
        elif custom_visible:
            self.view.set_custom_spectra_widget_hidden()
            self.spectra_valid = True
            self.on_groupingfile_changed()
            self.view.set_custom_groupingfile_widget_visible()
        elif crop_visible:
            self.view.set_custom_groupingfile_widget_hidden()
            self.groupingfile_valid = True
            self.on_spectra_changed(self.view.get_custom_spectra_text())
            self.view.set_custom_spectra_widget_visible()

    def set_invalid_spectra_status(self, text):
        if text:
            self.view.set_crop_invalid_indicator_visible(text)
            self.spectra_valid = False
        else:
            self.view.set_crop_invalid_indicator_hidden()
            self.spectra_valid = True
