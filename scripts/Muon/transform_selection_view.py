from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from Muon import widget_helper


class TransformSelectionView(QtGui.QWidget):
    # signals
    changeMethodSignal = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        super(TransformSelectionView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.methods=QtGui.QComboBox()
        options=["FFT","MaxEnt"]
        self.methods.addItems(options)
        self.grid.addWidget(self.methods)
        
 
