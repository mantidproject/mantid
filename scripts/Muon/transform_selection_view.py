from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui


class TransformSelectionView(QtGui.QWidget):
    """
    Create the transformation selection widget's appearance
    """
    # signals
    changeMethodSignal = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        super(TransformSelectionView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.methods = QtGui.QComboBox()
        # default to FFT
        options=["FFT"]
        self.methods.addItems(options)
        self.grid.addWidget(self.methods)
        self.methods.currentIndexChanged.connect(self.sendSignal)

    # sets the methods in the selection widget
    def setMethodsCombo(self,options):
        self.methods.clear()
        self.methods.addItems(options)

    def sendSignal(self,index):
        self.changeMethodSignal.emit(index)
