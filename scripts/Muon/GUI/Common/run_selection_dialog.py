# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets, QtCore, QtGui


class RunSelectionDialog(QtWidgets.QDialog):
    def __init__(self, current_runs, instrument, parent=None):
        QtWidgets.QDialog.__init__(self, parent)

        self.setWindowTitle('Run Selection')
        layout = QtWidgets.QVBoxLayout(self)

        self.message = QtWidgets.QLabel()
        self.message.setText('Which run do you wish to use for calculation?')
        layout.addWidget(self.message)

        current_runs_as_string = [instrument + str(run[0]) for run in current_runs]
        self.run_selector_combo = QtWidgets.QComboBox()
        self.run_selector_combo.addItems(current_runs_as_string)

        layout.addWidget(self.run_selector_combo)

        buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel,
            QtCore.Qt.Horizontal, self)

        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        self.setLayout(layout)

    def run(self):
        return self.run_selector_combo.currentText()

    def index(self):
        return self.run_selector_combo.currentIndex()

    @staticmethod
    def get_run(current_runs, instrument, parent=None):
        dialog = RunSelectionDialog(current_runs, instrument, parent)
        result = dialog.exec_()
        run = dialog.run()
        index = dialog.index()
        return (run, index, result == QtWidgets.QDialog.Accepted)
