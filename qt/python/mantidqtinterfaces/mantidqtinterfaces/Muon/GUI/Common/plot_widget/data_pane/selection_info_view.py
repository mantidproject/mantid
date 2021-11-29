# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableWidget import SequentialTableWidget


class SelectionInfoView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(SelectionInfoView, self).__init__(parent)
        self._selection_layout = QtWidgets.QVBoxLayout()

        self.selection_table = SequentialTableWidget(parent)
        self._selection_layout.addWidget(self.selection_table.widget)

        self.setLayout(self._selection_layout)
        self.setWindowFlags(QtCore.Qt.Dialog)

        self.setWindowTitle("plot selection")
        self.show()
        self.raise_()

    def cancel_pressed(self):
        self.hide()

    def apply_pressed(self):
        print("hi you pressed apply")
        self.hide()
