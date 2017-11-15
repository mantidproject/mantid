from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal

import ui_summation_settings_widget
from sans.gui_logic.models.add_runs_model import BinningType

class SummationSettingsWidget(QtGui.QWidget, ui_summation_settings_widget.Ui_SummationSettingsWidget):
    binningTypeChanged = pyqtSignal(int)
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
