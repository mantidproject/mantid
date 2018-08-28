from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
from qtpy.QtCore import Signal


class DummyView(QtWidgets.QWidget):
    buttonSignal = Signal(object)

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
