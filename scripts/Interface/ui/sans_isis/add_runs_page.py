from PyQt4 import QtGui
from PyQt4.QtCore import pyqtSignal
import ui_add_runs_page


class AddRunsPage(QtGui.QWidget, ui_add_runs_page.Ui_AddRunsPage):
    sum = pyqtSignal()
    outFileChanged = pyqtSignal()

    def __init__(self, parent=None):
        super(AddRunsPage, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def _connect_signals(self):
        self.sumButton.pressed.connect(self.sum)
        self.fileNameEdit.editingFinished.connect(self.outFileChanged)

    def run_selector_view(self):
        return self.run_selector

    def out_file_name(self):
        return str(self.fileNameEdit.text())

    def set_out_file_directory(self, out_file_directory):
        self.outputDirectoryLabel.setText("Output Directory: {}".format(out_file_directory))

    def no_save_directory(self):
        QtGui.QMessageBox.warning(self, "No Save Directory Set!",
                                  "You must set the mantid output directory before suming files.")

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

    def setupUi(self, other):
        ui_add_runs_page.Ui_AddRunsPage.setupUi(self, other)
