""" View for the masking table.

The view for the masking table displays all available masks for a SANS reduction. It also allows to display the moved
and masked SANS workspace.
"""

from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from six import with_metaclass

import ui_add_runs_page
from PyQt4.QtCore import pyqtSignal
from sans.gui_logic.models.add_runs_model import BinningType

class AddRunsPage(QtGui.QWidget, ui_add_runs_page.Ui_AddRunsPage):
    manageDirectoriesPressed = pyqtSignal()
    browsePressed = pyqtSignal()
    addRunsPressed = pyqtSignal()
    removeRunsPressed = pyqtSignal()
    removeAllRunsPressed = pyqtSignal()
    binningTypeChanged = pyqtSignal(int)
    preserveEventsChanged = pyqtSignal(bool)
    sumPressed = pyqtSignal()

    def __init__(self, parent=None):
        super(AddRunsPage, self).__init__(parent)
        self.setupUi(self)
        self.connect_signals()

    def setupUi(self, other):
        ui_add_runs_page.Ui_AddRunsPage.setupUi(self, other)
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
        self.addFileButton.pressed.connect(self.on_add_pressed)
        self.removeFileButton.pressed.connect(self.on_remove_pressed)
        self.removeAllFilesButton.pressed.connect(self.on_remove_all_pressed)
        self.manageDirectoriesButton.pressed.connect(self.on_manage_directories)
        self.browseFileButton.pressed.connect(self.on_browse_files)

    def binning_type_index_to_type(self, index):
        if index == 0:
            return BinningType.Custom
        elif index == 1:
            return BinningType.FromMonitors
        elif index == 2:
            return BinningType.SaveAsEventData

    def run_list(self):
        return self.fileListLineEdit.text()

    def selected_runs(self):
        return [runModel.row() for runModel in self.fileList.selectedIndexes()]

    def draw_runs(self, runs):
        model = QtGui.QStandardItemModel()
        for run in runs:
            item = QtGui.QStandardItem(run)
            model.appendRow(item)
        self.fileList.setModel(model)

    def on_binning_type_changed(self, index):
        binning_type = self.binning_type_index_to_type(index)
        self.binningTypeChanged.emit(binning_type)

    def on_overlay_ews_changed(self, state):
        self.preserveEventsChanged.emit(state != 0)

    def on_add_pressed(self):
        self.addRunsPressed.emit()

    def on_remove_all_pressed(self):
        self.removeAllRunsPressed.emit()

    def on_remove_pressed(self):
        self.removeRunsPressed.emit()

    def on_manage_directories(self):
        self.manageDirectoriesPressed.emit()

    def on_browse_files(self):
        self.browsePressed.emit()
