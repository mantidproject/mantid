from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui


class TransformSelectionView(QtGui.QWidget):
    # signals
    changeMethodSignal = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        super(TransformSelectionView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.methods = QtGui.QComboBox()
        options=["FFT"]
        self.methods.addItems(options)
        self.grid.addWidget(self.methods)
        self.methods.currentIndexChanged.connect(self.sendSignal)

    def setMethodsCombo(self,options):
        self.methods.clear()
        self.methods.addItems(options)

    def sendSignal(self,index):
        self.changeMethodSignal.emit(index)
