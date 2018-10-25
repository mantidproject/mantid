from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtWidgets


class SelectSubplot(QtWidgets.QDialog):

    subplotSelectorSignal = QtCore.Signal(object)
    closeEventSignal = QtCore.Signal()

    def __init__(self, subplots, parent=None):
        super(SelectSubplot, self).__init__()

        self.grid = QtWidgets.QGridLayout()
        self.combo = QtWidgets.QComboBox()
        self.combo.addItems(subplots)
        self.grid.addWidget(self.combo)

        btn = QtWidgets.QPushButton("ok")
        self.grid.addWidget(btn)
        self.setLayout(self.grid)
        self.setWindowTitle("Edit Lines Subplot Selector")
        btn.clicked.connect(self.buttonClick)

    def closeEvent(self, event):
        self.closeEventSignal.emit()

    def buttonClick(self):
        pick = self.combo.currentText()
        self.subplotSelectorSignal.emit(pick)
