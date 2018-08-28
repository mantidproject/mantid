from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
from qtpy.QtCore import Signal


class TransformSelectionView(QtWidgets.QWidget):

    """
    Create the transformation selection widget's appearance
    """
    # signals
    changeMethodSignal = Signal(object)

    def __init__(self, parent=None):
        super(TransformSelectionView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)
        self.methods = QtWidgets.QComboBox()
        # default to FFT
        options = ["FFT", "MaxEnt"]
        self.methods.addItems(options)
        self.grid.addWidget(self.methods)
        self.methods.currentIndexChanged.connect(self.sendSignal)

    def getLayout(self):
        return self.grid

        # sets the methods in the selection widget
    def setMethodsCombo(self, options):
        self.methods.clear()
        self.methods.addItems(options)

    def sendSignal(self):
        self.changeMethodSignal.emit(self.methods.currentText())
