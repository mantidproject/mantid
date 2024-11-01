# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from workbench.widgets.settings.fitting.view import FittingSettingsView
from workbench.widgets.settings.fitting.fitting_settings_model import FittingSettingsModel
from workbench.widgets.settings.view_utilities.settings_view_utilities import filter_out_mousewheel_events_from_combo_or_spin_box

from qtpy.QtCore import Qt


class FittingSettings(object):
    def __init__(self, parent, view=None, model=None):
        self.view = view if view else FittingSettingsView(parent, self)
        self.parent = parent
        self.model = model if model else FittingSettingsModel()
        self.add_filters()
        self.add_items_to_combo_boxes()
        self.load_current_setting_values()
        self.setup_signals()

    def add_filters(self):
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.auto_bkg)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.default_peak)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.findpeaks_fwhm)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.findpeaks_tol)

    def add_items_to_combo_boxes(self):
        self.view.auto_bkg.addItem("None")
        for name in self.model.get_background_function_names():
            self.view.auto_bkg.addItem(name)
        for name in self.model.get_peak_function_names():
            self.view.default_peak.addItem(name)

    def load_current_setting_values(self):
        background = self.model.get_auto_background().split(" ", 1)
        if not background[0]:
            background[0] = "None"
        if self.view.auto_bkg.findText(background[0], Qt.MatchExactly) != -1:
            self.view.auto_bkg.setCurrentText(background[0])
        else:
            self.view.auto_bkg.setCurrentText("LinearBackground")
        if len(background) > 1:
            self.view.background_args.setText(background[1])
        else:
            self.view.background_args.clear()

        default_peak = self.model.get_current_peak()
        if self.view.default_peak.findText(default_peak, Qt.MatchExactly) != -1:
            self.view.default_peak.setCurrentText(default_peak)
        else:
            self.view.auto_bkg.setCurrentText("Gaussian")

        fwhm = self.model.get_fwhm()
        if fwhm == "":
            self.view.findpeaks_fwhm.setValue(7)
        else:
            self.view.findpeaks_fwhm.setValue(int(fwhm))

        tolerance = self.model.get_tolerance()
        if tolerance == "":
            self.view.findpeaks_tol.setValue(4)
        else:
            self.view.findpeaks_tol.setValue(int(tolerance))

    def setup_signals(self):
        self.view.auto_bkg.currentTextChanged.connect(self.action_auto_background_changed)
        self.view.background_args.editingFinished.connect(self.action_background_args_changed)
        self.view.default_peak.currentTextChanged.connect(self.action_default_peak_changed)
        self.view.findpeaks_fwhm.valueChanged.connect(self.action_find_peaks_fwhm_changed)
        self.view.findpeaks_tol.valueChanged.connect(self.action_find_peaks_tolerance_changed)

    def action_auto_background_changed(self, item_name):
        if item_name == "None":
            self.model.set_auto_background("")
            return
        background_string = item_name + " " + self.view.background_args.text()
        self.model.set_auto_background(background_string)

    def action_background_args_changed(self):
        if self.view.auto_bkg.currentText() == "None":
            self.model.set_auto_background("")
        else:
            background_string = self.view.auto_bkg.currentText() + " " + self.view.background_args.text()
            self.model.set_auto_background(background_string)

    def action_default_peak_changed(self, item_name):
        self.model.set_default_peak(item_name)

    def action_find_peaks_fwhm_changed(self, value):
        self.model.set_fwhm(str(value))

    def action_find_peaks_tolerance_changed(self, value):
        self.model.set_tolerance(str(value))

    def update_properties(self):
        self.load_current_setting_values()
        self.action_background_args_changed()
