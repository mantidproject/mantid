# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets


class DummyView(QtWidgets.QWidget):
    buttonSignal = QtCore.pyqtSignal(object)

    def __init__(self, name, parent=None):
        super(DummyView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)
        self.message = name
        btn = QtWidgets.QPushButton(name, self)
        self.grid.addWidget(btn)
        btn.clicked.connect(self.buttonClick)

    def buttonClick(self):
        self.buttonSignal.emit(self.message)

    def getLayout(self):
        return self.grid
