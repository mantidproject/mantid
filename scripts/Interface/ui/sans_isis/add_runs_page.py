# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Signal

from mantidqt.utils.qt import load_ui

Ui_AddRunsPage, _ = load_ui(__file__, "add_runs_page.ui")


class AddRunsPage(QtWidgets.QWidget, Ui_AddRunsPage):
    sum = Signal()
    customOutFileChanged = Signal(bool)
    saveDirectoryClicked = Signal()

    def __init__(self, parent=None):
        super(AddRunsPage, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def _connect_signals(self):
        self.sumButton.pressed.connect(self.sum)
        self.customFilenameCheckBox.stateChanged.connect(self._handle_custom_filename_checked)
        self.saveDirectoryButton.clicked.connect(self.saveDirectoryClicked)

    def _handle_custom_filename_checked(self, enabled):
        self.customOutFileChanged.emit(enabled)

    def run_selector_view(self):
        return self.run_selector

    def out_file_name(self):
        return str(self.fileNameEdit.text())

    def set_out_file_directory(self, out_file_directory):
        self.outputDirectoryLabel.setText("Save Directory: {}".format(out_file_directory))

    def no_save_directory(self):
        QtWidgets.QMessageBox.warning(self, "No Save Directory Set!", "You must set the mantid output directory before suming files.")

    def set_out_file_name(self, out_file_name):
        self.fileNameEdit.setText(out_file_name)

    def summation_settings_view(self):
        return self.summation_settings

    def enable_sum(self):
        self.sumButton.setEnabled(True)

    def disable_sum(self):
        self.sumButton.setEnabled(False)

    def enable_summation_settings(self):
        self.summation_settings_view().setEnabled(True)

    def disable_summation_settings(self):
        self.summation_settings_view().setEnabled(False)

    def enable_output_file_name_edit(self):
        self.fileNameEdit.setEnabled(True)

    def disable_output_file_name_edit(self):
        self.fileNameEdit.setEnabled(False)

    def clear_output_file_name_edit(self):
        self.fileNameEdit.clear()

    def display_save_directory_box(self, title, default_path):
        filename = QtWidgets.QFileDialog.getExistingDirectory(self, title, default_path, QtWidgets.QFileDialog.ShowDirsOnly)
        return filename

    def setupUi(self, other):
        Ui_AddRunsPage.setupUi(self, other)
