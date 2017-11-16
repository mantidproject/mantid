from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal

import ui_summation_settings_widget
from sans.gui_logic.models.binning_type import BinningType
import types

class SummationSettingsWidget(QtGui.QWidget, ui_summation_settings_widget.Ui_SummationSettingsWidget):
    binningTypeChanged = pyqtSignal(BinningType)
    preserveEventsChanged = pyqtSignal(bool)
    sum = pyqtSignal()

    def __init__(self, parent=None):
        super(SummationSettingsWidget, self).__init__(parent)
        self.setupUi(self)
        self.connect_signals()

    def setupUi(self, other):
        ui_summation_settings_widget.Ui_SummationSettingsWidget.setupUi(self, other)
        self.setupBinningTypes()

    def setupBinningTypes(self):
        binningTypes = [
            'Use custom binning',
            'Use binning from monitors',
            'Save as event data'
        ]
        for binningType in binningTypes:
            self.binningType.addItem(binningType)

    def connect_signals(self):
        self.binningType.currentIndexChanged.connect(self.on_binning_type_changed)
        self.overlayEventWorkspacesCheckbox.stateChanged.connect(self.on_overlay_ews_changed)

    def binning_type_index_to_type(self, index):
        if index == 0:
            return BinningType.Custom
        elif index == 1:
            return BinningType.FromMonitors
        elif index == 2:
            return BinningType.SaveAsEventData

    def on_binning_type_changed(self, index):
        binning_type = self.binning_type_index_to_type(index)
        self.binningTypeChanged.emit(binning_type)

    def on_overlay_ews_changed(self, state):
        self.preserveEventsChanged.emit(state != 0)

    def apply_settings(self, settings):
        self.apply_bin_settings(settings)
        self.apply_additional_time_shifts(settings)
        self.apply_overlay_event_workspaces(settings)

    def bin_settings(self):
        return self.customBinBoundariesLineEdit.text()

    def additional_time_shifts(self):
        return self.customBinBoundariesLineEdit.text()

    def apply_overlay_event_workspaces(self, settings):
        if settings.has_overlay_event_workspaces():
            self.overlayEventWorkspacesCheckbox.setEnabled(True)
            should_be_checked = settings.is_overlay_event_workspaces_enabled()
            #self.overlayEventWorkspacesCheckbox.setChecked(should_be_checked)
        else:
            #self.overlayEventWorkspacesCheckbox.setChecked(False)
            self.overlayEventWorkspacesCheckbox.setEnabled(False)

    def disable_and_clear_text(self):
        print("Disable and clear!")
        self.customBinBoundariesLineEdit.setEnabled(False)
        self.customBinBoundariesLineEdit.setText('')

    def apply_bin_settings(self, settings):
        if settings.has_bin_settings():
            self.customBinBoundariesLineEdit.setEnabled(True)
            self.customBinBoundariesLineEdit.setText(settings.bin_settings)
        elif not settings.has_additional_time_shifts():
            self.disable_and_clear_text()

    def apply_additional_time_shifts(self, settings):
        if settings.has_additional_time_shifts():
            self.customBinBoundariesLineEdit.setEnabled(True)
            self.customBinBoundariesLineEdit.setText(settings.additional_time_shifts)
        elif not settings.has_bin_settings():
            self.disable_and_clear_text()
