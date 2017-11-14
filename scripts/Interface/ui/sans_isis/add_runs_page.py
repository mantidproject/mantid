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
from mantidqtpython import MantidQt

class AddRunsPage(QtGui.QWidget, ui_add_runs_page.Ui_AddRunsPage):
    manageDirectories = pyqtSignal()
    browse = pyqtSignal()
    addRuns = pyqtSignal()
    removeRuns = pyqtSignal()
    removeAllRuns = pyqtSignal()
    binningTypeChanged = pyqtSignal(int)
    preserveEventsChanged = pyqtSignal(bool)
    sum = pyqtSignal()

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
        self.fileList.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

    def connect_signals(self):
        self.binningType.currentIndexChanged.connect(self.on_binning_type_changed)
        self.overlayEventWorkspacesCheckbox.stateChanged.connect(self.on_overlay_ews_changed)
        self.addFileButton.pressed.connect(self.on_add_pressed)
        self.fileListLineEdit.returnPressed.connect(self.on_add_pressed)
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

    def show_directories_manager(self):
        MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)

    def filter_for_extensions(self, extensions):
        if extensions:
            return "Files ( *" + " *".join(extensions) + ")"
        else:
            return "Files ()"

    def previous_directory_settings(self):
        previous_directories = QtCore.QSettings()
        previous_directories.beginGroup("CustomInterfaces/SANSRunWindow/AddRuns")
        return previous_directories

    def run_not_found(self):
        QtGui.QMessageBox.warning(self, "Run Not Found!",\
            "Could not find one or more of the runs specified.")

    def invalid_run_query(self, message):
        QtGui.QMessageBox.warning(self, "Invalid Run Query!",\
            message)

    def previous_or_default_directory(self, settings, default):
        directory = settings.value("InPath", default)

    def store_previous_directory(self, settings, path):
        previous_file = QtCore.QFileInfo(path)
        settings.setValue("InPath", previous_file.absoluteDir().absolutePath())

    def show_file_picker(self, extensions, search_directories):
        previous_directories = self.previous_directory_settings()
        default_directory = search_directories[0]
        directory = self.previous_or_default_directory(previous_directories, default_directory)
        file_filter = self.filter_for_extensions(extensions)
        chosen_files = QtGui.QFileDialog.getOpenFileNames(self, "Select files", directory, file_filter)
        if chosen_files:
            self.store_previous_directory(previous_directories, chosen_files[0])
        return chosen_files

    def run_list(self):
        return self.fileListLineEdit.text()

    def selected_runs(self):
        selected = [runModel.row() for runModel in\
                    self.fileList.selectedIndexes()]
        return selected

    def draw_runs(self, runs):
        model = QtGui.QStandardItemModel()
        for run in runs:
            item = QtGui.QStandardItem(run.display_name())
            item.setToolTip(run.file_path())
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
