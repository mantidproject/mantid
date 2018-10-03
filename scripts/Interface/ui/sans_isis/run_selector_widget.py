# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore

import ui_run_selector_widget
from PyQt4.QtCore import pyqtSignal
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
        self._connect_signals()

    def setupUi(self, other):
        ui_run_selector_widget.Ui_RunSelectorWidget.setupUi(self, other)
        self.runList.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

    def show_file_picker(self, extensions, search_directories):
        assert(len(extensions) > 0)
        previous_directories = self._previous_directory_settings()
        default_directory = search_directories[0]
        directory = self._previous_or_default_directory(previous_directories, default_directory)
        file_filter = self._filter_for_extensions(extensions)
        chosen_files = QtGui.QFileDialog.getOpenFileNames(self, "Select files", directory, file_filter)
        if chosen_files:
            self._store_previous_directory(previous_directories, chosen_files[0])
        return [str(chosen_file) for chosen_file in chosen_files]

    def _previous_directory_settings(self):
        previous_directories = QtCore.QSettings()
        previous_directories.beginGroup("CustomInterfaces/SANSRunWindow/AddRuns")
        return previous_directories

    def _previous_or_default_directory(self, settings, default):
        return settings.value("InPath", default)

    def _store_previous_directory(self, settings, path):
        previous_file = QtCore.QFileInfo(path)
        settings.setValue("InPath", previous_file.absoluteDir().absolutePath())

    def _filter_for_extensions(self, extensions):
        return "Files ( *" + " *".join(extensions) + ")"

    def show_directories_manager(self):
        MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)

    def run_not_found(self):
        QtGui.QMessageBox.warning(self, "Run Not Found!",
                                  "Could not find one or more of the runs specified.")

    def invalid_run_query(self, message):
        QtGui.QMessageBox.warning(self, "Invalid Run Query!", message)

    def run_list(self):
        return str(self.runLineEdit.text())

    def selected_runs(self):
        selected = [runModel.row() for runModel in
                    self.runList.selectedIndexes()]
        return selected

    def draw_runs(self, runs):
        model = QtGui.QStandardItemModel()
        for run in runs:
            item = QtGui.QStandardItem(run.display_name())
            item.setToolTip(run.file_path())
            model.appendRow(item)
        self.runList.setModel(model)

    @property
    def title(self):
        self.runsGroup.getTitle()

    @title.setter
    def title(self, new_title):
        self.runsGroup.setTitle(new_title)

    def _handle_add_run(self):
        self.addRuns.emit()

    def _handle_remove_all_runs(self):
        self.removeAllRuns.emit()

    def _handle_remove_run(self):
        self.removeRuns.emit()

    def _handle_manage_directories(self):
        self.manageDirectories.emit()

    def _handle_browse_files(self):
        self.browse.emit()

    def _connect_signals(self):
        self.addRunButton.pressed.connect(self._handle_add_run)
        self.runLineEdit.returnPressed.connect(self._handle_add_run)
        self.removeRunButton.pressed.connect(self._handle_remove_run)
        self.removeAllRunsButton.pressed.connect(self._handle_remove_all_runs)
        self.manageDirectoriesButton.pressed.connect(self._handle_manage_directories)
        self.browseFileButton.pressed.connect(self._handle_browse_files)
