from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets


class DummyLabelView(QtWidgets.QWidget):

    def __init__(self, name, parent=None):
        super(DummyLabelView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)

        self.label = QtWidgets.QLabel(name)
        self.grid.addWidget(self.label)

    def getLayout(self):
        return self.grid

    def updateLabel(self, message):
        self.label.setText("The " + message + " has been pressed")
