from __future__ import (absolute_import, division, print_function)


from PyQt4 import QtCore
from PyQt4 import QtGui


class DummyView(QtGui.QWidget):

    buttonSignal = QtCore.pyqtSignal(object)

    def __init__(self, name, parent=None):
        super(DummyView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.message = name
        btn = QtGui.QPushButton(name, self)
        self.grid.addWidget(btn)
        btn.clicked.connect(self.buttonClick)

    def buttonClick(self):
        self.buttonSignal.emit(self.message)

    def getLayout(self):
        return self.grid
