# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from mantid.api import FunctionFactory
from mantid.kernel import ConfigService
from workbench.widgets.settings.fitting.view import FittingSettingsView

from qtpy.QtCore import Qt
from enum import Enum


class FittingProperties(Enum):
    AUTO_BACKGROUND = "curvefitting.autoBackground"
    DEFAULT_PEAK = "curvefitting.defaultPeak"
    FWHM = "curvefitting.findPeaksFWHM"
    TOLERANCE = "curvefitting.findPeaksTolerance"


class FittingSettings(object):

    def __init__(self, parent, view=None):
        self.view = view if view else FittingSettingsView(parent, self)
        self.parent = parent
        self.add_items_to_combo_boxes()
        self.load_current_setting_values()
        self.setup_signals()

    def add_items_to_combo_boxes(self):
        fun_factory = FunctionFactory.Instance()
        self.view.auto_bkg.addItem("None")
        for name in fun_factory.getBackgroundFunctionNames():
            self.view.auto_bkg.addItem(name)
        for name in fun_factory.getPeakFunctionNames():
            self.view.default_peak.addItem(name)

    def load_current_setting_values(self):
        background = ConfigService.getString(FittingProperties.AUTO_BACKGROUND.value).split(" ", 1)
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

        default_peak = ConfigService.getString(FittingProperties.DEFAULT_PEAK.value)
        if self.view.default_peak.findText(default_peak, Qt.MatchExactly) != -1:
            self.view.default_peak.setCurrentText(default_peak)
        else:
            self.view.auto_bkg.setCurrentText("Gaussian")

        fwhm = ConfigService.getString(FittingProperties.FWHM.value)
        if fwhm == "":
            self.view.findpeaks_fwhm.setValue(7)
        else:
            self.view.findpeaks_fwhm.setValue(int(fwhm))

        tolerance = ConfigService.getString(FittingProperties.TOLERANCE.value)
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
            ConfigService.setString(FittingProperties.AUTO_BACKGROUND.value, "")
            return
        background_string = item_name + " " + self.view.background_args.text()
        ConfigService.setString(FittingProperties.AUTO_BACKGROUND.value, background_string)

    def action_background_args_changed(self):
        if self.view.auto_bkg.currentText() == "None":
            ConfigService.setString(FittingProperties.AUTO_BACKGROUND.value, "")
        else:
            background_string = self.view.auto_bkg.currentText() + " " + self.view.background_args.text()
            ConfigService.setString(FittingProperties.AUTO_BACKGROUND.value, background_string)

    def action_default_peak_changed(self, item_name):
        ConfigService.setString(FittingProperties.DEFAULT_PEAK.value, item_name)

    def action_find_peaks_fwhm_changed(self, value):
        ConfigService.setString(FittingProperties.FWHM.value, str(value))

    def action_find_peaks_tolerance_changed(self, value):
        ConfigService.setString(FittingProperties.TOLERANCE.value, str(value))

    def update_properties(self):
        self.load_current_setting_values()
        self.action_background_args_changed()
