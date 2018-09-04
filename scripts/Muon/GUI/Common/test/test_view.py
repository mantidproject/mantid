from __future__ import (absolute_import, division, print_function)


from PyQt4 import QtCore
from PyQt4 import QtGui

from Muon.GUI.Common import table_utils

class TestView(QtGui.QWidget):

    buttonSignal = QtCore.pyqtSignal()

    def __init__(self,parent=None):
        super(TestView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)

        self.table = QtGui.QTableWidget(self)
        self.table.resize(800, 800)
        self.table.setRowCount(4)
        self.table.setColumnCount(4)
        self.table.setColumnWidth(0, 300)
        self.table.setColumnWidth(1, 300)
        self.table.setColumnWidth(2, 300)
        self.table.setColumnWidth(3, 300)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(
            ("Property;Value 1; Value 2; Value3").split(";"))
        # populate table
        options = ['Fwd',"Bwd"]

        table_utils.setRowName(self.table, 0, "Workspaces")
        self.ws0 = table_utils.addDoubleToTable(self.table,"",0,1)
        self.ws1 = table_utils.addDoubleToTable(self.table,"",0,2)
        self.ws2 = table_utils.addDoubleToTable(self.table,"",0,3)

        #self.ws = table_utils.addComboToTable(self.FFTTable, 0, options)
 




        btn = QtGui.QPushButton("print context", self)

        self.grid.addWidget(self.table)
        self.grid.addWidget(btn)
        btn.clicked.connect(self.buttonClick)

    def buttonClick(self):
        self.buttonSignal.emit()

    def getLayout(self):
        return self.grid
