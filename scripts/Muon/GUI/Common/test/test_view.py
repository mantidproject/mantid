from __future__ import (absolute_import, division, print_function)


from PyQt4 import QtCore
from PyQt4 import QtGui

from Muon.GUI.Common import table_utils

class TestView(QtGui.QWidget):

    buttonSignal = QtCore.pyqtSignal()

    def __init__(self,context, parent=None):
        super(TestView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)

        self.table = QtGui.QTableWidget(self)
        self.table.resize(800, 800)
        self.table.setRowCount(2)
        self.table.setColumnCount(4)
        self.table.setColumnWidth(0, 300)
        self.table.setColumnWidth(1, 100)
        self.table.setColumnWidth(2, 100)
        self.table.setColumnWidth(3, 100)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(
            ("Property;Value 1; Value 2; Value3").split(";"))
        # populate table
        options = ['Fwd',"Bwd"]

        table_utils.setRowName(self.table, 0, "Groups")
 
        group_name = context
        print(group_name)

        self.ws0 = table_utils.addDoubleToTable(self.table,group_name[0],0,1)
        self.ws1 = table_utils.addDoubleToTable(self.table,group_name[1],0,2)
        self.ws2 = table_utils.addDoubleToTable(self.table,group_name[2],0,3)

        table_utils.setRowName(self.table,1, "Pair")
        options = ["a","b","c"]
        self.g1 = table_utils.addComboToTable(self.table, 1, options,1)
        self.g2 = table_utils.addComboToTable(self.table, 1, options,2)
        self.alpha = table_utils.addDoubleToTable(self.table,"1.",1,3)
 




        btn = QtGui.QPushButton("print context", self)

        self.grid.addWidget(self.table)
        self.grid.addWidget(btn)
        btn.clicked.connect(self.buttonClick)

    def buttonClick(self):
        self.buttonSignal.emit()

    def getLayout(self):
        return self.grid
