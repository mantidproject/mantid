from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from six import with_metaclass

import ui_run_selector_widget
from PyQt4.QtCore import pyqtSignal
from sans.gui_logic.models.binning_type import BinningType
from mantidqtpython import MantidQt

class RunSelectorWidget(QtGui.QWidget, ui_run_selector_widget.Ui_RunSelectorWidget):
    manageDirectories = pyqtSignal()
    browse = pyqtSignal()
    addRuns = pyqtSignal()
    removeRuns = pyqtSignal()
    removeAllRuns = pyqtSignal()

    def __init__(self, parent=None):
        super(RunSelectorWidget, self).__init__(parent)
        self.setupUi(self)
        self.connect_signals()

    def setupUi(self, other):
        ui_run_selector_widget.Ui_RunSelectorWidget.setupUi(self, other)
        self.runList.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

    def connect_signals(self):
        self.addRunButton.pressed.connect(self.on_add_pressed)
        self.runLineEdit.returnPressed.connect(self.on_add_pressed)
        self.removeRunButton.pressed.connect(self.on_remove_pressed)
        self.removeAllRunsButton.pressed.connect(self.on_remove_all_pressed)
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
        return self.runLineEdit.text()

    def selected_runs(self):
        selected = [runModel.row() for runModel in\
                    self.runList.selectedIndexes()]
        return selected

    def draw_runs(self, runs):
        model = QtGui.QStandardItemModel()
        for run in runs:
            item = QtGui.QStandardItem(run.display_name())
            item.setToolTip(run.file_path())
            model.appendRow(item)
        self.runList.setModel(model)

    def on_add_pressed(self):
        self.addRuns.emit()

    def on_remove_all_pressed(self):
        self.removeAllRuns.emit()

    def on_remove_pressed(self):
        self.removeRuns.emit()

    def on_manage_directories(self):
        self.manageDirectories.emit()

    def on_browse_files(self):
        self.browse.emit()
