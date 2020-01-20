# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from __future__ import absolute_import, unicode_literals

from mantid.api import FunctionFactory, IPeakFunction
from mantid.kernel import ConfigService
from workbench.widgets.settings.fitting.view import FittingSettingsView

from qtpy.QtCore import Qt


class FittingSettings(object):
    AUTO_BACKGROUND = "curvefitting.autoBackground"
    DEFAULT_PEAK = "curvefitting.defaultPeak"
    FWHM = "curvefitting.findpeaksFWHM"
    TOLERANCE = "curvefitting.findpeaksTolerance"

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
        background = ConfigService.getString(self.AUTO_BACKGROUND).split(" ", 1)
        if self.view.auto_bkg.findText(background[0], Qt.MatchExactly) != -1:
            self.view.auto_bkg.setCurrentText(background[0])
        else:
            self.view.auto_bkg.setCurrentText("LinearBackground")
        if len(background) > 1:
            self.view.background_args.setText(background[1])

        default_peak = ConfigService.getString(self.DEFAULT_PEAK)
        if self.view.default_peak.findText(default_peak, Qt.MatchExactly) != -1:
            self.view.default_peak.setCurrentText(default_peak)
        else:
            self.view.auto_bkg.setCurrentText("Gaussian")

        fwhm = ConfigService.getString(self.FWHM)
        if fwhm == "":
            self.view.findpeaks_fwhm.setValue(7)
        else:
            self.view.findpeaks_fwhm.setValue(int(fwhm))

        tolerance = ConfigService.getString(self.TOLERANCE)
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
        if item_name is "None":
            ConfigService.setString(self.AUTO_BACKGROUND, "")
            return
        background_string = item_name + " " + self.view.background_args.text()
        ConfigService.setString(self.AUTO_BACKGROUND,  background_string)

    def action_background_args_changed(self):
        if self.view.auto_bkg.currentText() is "None":
            ConfigService.setString(self.AUTO_BACKGROUND, "")
        else:
            background_string = self.view.auto_bkg.currentText() + " " + self.view.background_args.text()
            ConfigService.setString(self.AUTO_BACKGROUND,  background_string)

    def action_default_peak_changed(self, item_name):
        ConfigService.setString(self.DEFAULT_PEAK, item_name)

    def action_find_peaks_fwhm_changed(self, value):
        ConfigService.setString(self.FWHM, str(value))

    def action_find_peaks_tolerance_changed(self, value):
        ConfigService.setString(self.TOLERANCE, str(value))
