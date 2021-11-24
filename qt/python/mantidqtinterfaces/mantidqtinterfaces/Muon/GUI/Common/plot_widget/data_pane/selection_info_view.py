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

        self._button_layout = QtWidgets.QHBoxLayout()

        self._apply = QtWidgets.QPushButton(text="Apply", parent=self)
        self._apply.clicked.connect(self.apply_pressed)
        self._button_layout.addWidget(self._apply)

        self._cancel = QtWidgets.QPushButton(text="Cancel", parent=self)
        self._cancel.clicked.connect(self.cancel_pressed)
        self._button_layout.addWidget(self._cancel)

        self._selection_layout.addLayout(self._button_layout)

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
