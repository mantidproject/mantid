# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from workbench.widgets.settings.base_classes.config_settings_presenter import SettingsPresenterBase
from workbench.widgets.settings.fitting.fitting_settings_model import FittingSettingsModel
from workbench.widgets.settings.fitting.view import FittingSettingsView
from workbench.widgets.settings.view_utilities.settings_view_utilities import filter_out_mousewheel_events_from_combo_or_spin_box

from qtpy.QtCore import Qt


class FittingSettings(SettingsPresenterBase):
    def __init__(self, parent, model: FittingSettingsModel, view=None):
        super().__init__(parent, model)
        self._view = view if view else FittingSettingsView(parent, self)
        self._model = model
        self.add_filters()
        self.add_items_to_combo_boxes()
        self.load_current_setting_values()
        self.setup_signals()

    def get_view(self):
        return self._view

    def add_filters(self):
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.auto_bkg)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.default_peak)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.findpeaks_fwhm)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.findpeaks_tol)

    def add_items_to_combo_boxes(self):
        self._view.auto_bkg.addItem("None")
        for name in self._model.get_background_function_names():
            self._view.auto_bkg.addItem(name)
        for name in self._model.get_peak_function_names():
            self._view.default_peak.addItem(name)

    def load_current_setting_values(self):
        background = self._model.get_auto_background().split(" ", 1)
        if not background[0]:
            background[0] = "None"
        if self._view.auto_bkg.findText(background[0], Qt.MatchExactly) != -1:
            self._view.auto_bkg.setCurrentText(background[0])
        else:
            self._view.auto_bkg.setCurrentText("LinearBackground")
        if len(background) > 1:
            self._view.background_args.setText(background[1])
        else:
            self._view.background_args.clear()

        default_peak = self._model.get_default_peak()
        if self._view.default_peak.findText(default_peak, Qt.MatchExactly) != -1:
            self._view.default_peak.setCurrentText(default_peak)
        else:
            self._view.auto_bkg.setCurrentText("Gaussian")

        fwhm = self._model.get_fwhm()
        if fwhm == "":
            self._view.findpeaks_fwhm.setValue(7)
        else:
            self._view.findpeaks_fwhm.setValue(int(fwhm))

        tolerance = self._model.get_tolerance()
        if tolerance == "":
            self._view.findpeaks_tol.setValue(4)
        else:
            self._view.findpeaks_tol.setValue(int(tolerance))

    def setup_signals(self):
        self._view.auto_bkg.currentTextChanged.connect(self.action_auto_background_changed)
        self._view.background_args.editingFinished.connect(self.action_background_args_changed)
        self._view.default_peak.currentTextChanged.connect(self.action_default_peak_changed)
        self._view.findpeaks_fwhm.valueChanged.connect(self.action_find_peaks_fwhm_changed)
        self._view.findpeaks_tol.valueChanged.connect(self.action_find_peaks_tolerance_changed)

    def action_auto_background_changed(self, item_name):
        if item_name == "None":
            self._model.set_auto_background("")
        else:
            background_string = item_name + " " + self._view.background_args.text()
            self._model.set_auto_background(background_string)
        self.notify_changes()

    def action_background_args_changed(self):
        if self._view.auto_bkg.currentText() == "None":
            self._model.set_auto_background("")
        else:
            background_string = self._view.auto_bkg.currentText() + " " + self._view.background_args.text()
            self._model.set_auto_background(background_string)
        self.notify_changes()

    def action_default_peak_changed(self, item_name):
        self._model.set_default_peak(item_name)
        self.notify_changes()

    def action_find_peaks_fwhm_changed(self, value):
        self._model.set_fwhm(str(value))
        self.notify_changes()

    def action_find_peaks_tolerance_changed(self, value):
        self._model.set_tolerance(str(value))
        self.notify_changes()

    def update_properties(self):
        self.load_current_setting_values()
        self.action_background_args_changed()
