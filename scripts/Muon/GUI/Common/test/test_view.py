from __future__ import (absolute_import, division, print_function)


from PyQt4 import QtCore
from PyQt4 import QtGui

from Muon.GUI.Common import table_utils

class TestView(QtGui.QWidget):

    buttonSignal = QtCore.pyqtSignal()
    groupChangedSignal = QtCore.pyqtSignal(object)

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
        table_utils.setRowName(self.table, 0, "Groups")
 
        group_name =["a","b","c"]
        print(group_name)

        self.ws0 = table_utils.addDoubleToTable(self.table,group_name[0],0,1)
        self.ws1 = table_utils.addDoubleToTable(self.table,group_name[1],0,2)
        self.ws2 = table_utils.addDoubleToTable(self.table,group_name[2],0,3)

        table_utils.setRowName(self.table,1, "Pair")
        options = group_name
       
        self.g1 = table_utils.addComboToTable(self.table, 1, options,1)
        self.g2 = table_utils.addComboToTable(self.table, 1, options,2)

        self.alpha = table_utils.addDoubleToTable(self.table,"2.",1,3)

        self.loadFromContext(context)

        btn = QtGui.QPushButton("print context", self)

        self.grid.addWidget(self.table)
        self.grid.addWidget(btn)
        btn.clicked.connect(self.buttonClick)
        self.table.itemChanged.connect(self.groupChanged)

    def loadFromContext(self,context):
        self.table.blockSignals(True)
        group_name = context["Group Names"]
 
        self.ws0.setText(group_name[0]) 
        self.ws1.setText(group_name[1])
        self.ws2.setText(group_name[2])

        self.g1.clear()
        self.g2.clear()

        self.g1.addItems(group_name)
        self.g2.addItems(group_name)

        index = self.g1.findText(context["Pair_F"])
        self.g1.setCurrentIndex(index)

        index = self.g2.findText(context["Pair_B"])
        self.g2.setCurrentIndex(index)

        self.alpha.setText(str(context["Pair_alpha"]))
        self.table.blockSignals(False)

    def getSubContext(self):
        context = {}
        context["Group Names"] = [self.ws0.text(), self.ws1.text(),self.ws2.text()]
        context["Pair_F"] = str(self.g1.currentText())
        context["Pair_B"] = str(self.g2.currentText())
        context["Pair_alpha"] = float(self.alpha.text())
        return context

    def buttonClick(self):
        self.buttonSignal.emit()

    def groupChanged(self,cell):
        
        self.groupChangedSignal.emit(cell.row())

    def getLayout(self):
        return self.grid
