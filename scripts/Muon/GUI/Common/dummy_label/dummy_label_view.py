# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


from qtpy import QtWidgets


class DummyLabelView(QtWidgets.QWidget):

    def __init__(self, subcontext, parent=None):
        super(DummyLabelView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)

        self.label = QtWidgets.QLabel("none")
        self.grid.addWidget(self.label)
        self.loadFromContext(subcontext)

    def getLayout(self):
        return self.grid

    def updateLabel(self, message):
        self.label.setText("The " + message + " has been pressed")

    # interact with context
    def loadFromContext(self, subcontext):
        self.label.setText(subcontext["label"])

    def getSubContext(self):
        subcontext = {}
        subcontext["label"] = str(self.label.text())
        return subcontext
