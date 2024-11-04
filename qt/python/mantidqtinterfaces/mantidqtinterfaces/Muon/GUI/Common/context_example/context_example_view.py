# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets

from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils


class ContextExampleView(QtWidgets.QWidget):
    updateSignal = QtCore.pyqtSignal()
    groupChangedSignal = QtCore.pyqtSignal(object)

    def __init__(self, context, parent=None):
        super(ContextExampleView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)

        self.table = QtWidgets.QTableWidget(self)
        self.table.resize(800, 800)
        self.table.setRowCount(2)
        self.table.setColumnCount(4)
        self.table.setColumnWidth(0, 300)
        self.table.setColumnWidth(1, 100)
        self.table.setColumnWidth(2, 100)
        self.table.setColumnWidth(3, 100)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(("Property;Value 1; Value 2; Value3").split(";"))
        # populate table

        # row of groups
        table_utils.setRowName(self.table, 0, "Groups")
        group_name = ["a", "b", "c"]
        self.ws0, _ = table_utils.addDoubleToTable(self.table, group_name[0], 0, 1)
        self.ws1, _ = table_utils.addDoubleToTable(self.table, group_name[1], 0, 2)
        self.ws2, _ = table_utils.addDoubleToTable(self.table, group_name[2], 0, 3)

        # row to describe a pair
        table_utils.setRowName(self.table, 1, "Pair")
        self.g1 = table_utils.addComboToTable(self.table, 1, group_name, 1)
        self.g2 = table_utils.addComboToTable(self.table, 1, group_name, 2)
        self.alpha, _ = table_utils.addDoubleToTable(self.table, "2.", 1, 3)

        # explicit update button
        btn = QtWidgets.QPushButton("print context", self)

        # create grid
        self.grid.addWidget(self.table)
        self.grid.addWidget(btn)

        # add connections
        btn.clicked.connect(self.sendUpdateSignal)
        # needed for updating the possible pairs when groups change
        self.table.itemChanged.connect(self.groupChanged)
        # load values into GUI from context
        self.loadFromContext(context)

    # signals
    def sendUpdateSignal(self):
        self.updateSignal.emit()

    def groupChanged(self, cell):
        self.groupChangedSignal.emit(cell.row())

    # get layout
    def getLayout(self):
        return self.grid

    # context interaction
    def loadFromContext(self, context):
        """
        Create a simple dict of the values
        from the GUI. This does not alter
        how the information is stored
        """
        # make sure we dont fire signals during update
        self.table.blockSignals(True)

        # get the group names and update the GUI
        group_name = context["Group Names"]

        self.ws0.setText(group_name[0])
        self.ws1.setText(group_name[1])
        self.ws2.setText(group_name[2])

        # store the detectors
        self.dets = context["Group dets"]

        # update combo boxes (clear and repopulate)
        self.g1.clear()
        self.g2.clear()
        self.g1.addItems(group_name)
        self.g2.addItems(group_name)
        # set correct selection for pair
        index = self.g1.findText(context["Pair_F"])
        self.g1.setCurrentIndex(index)
        index = self.g2.findText(context["Pair_B"])
        self.g2.setCurrentIndex(index)
        # set alpha for pair
        self.alpha.setText(str(context["Pair_alpha"]))

        # turn signals back on
        self.table.blockSignals(False)

    def getSubContext(self):
        """
        This packs up the information
        from the GUI into a simple dict that can be translated
        into the context
        """
        context = {}
        context["Group Names"] = [self.ws0.text(), self.ws1.text(), self.ws2.text()]
        context["Group dets"] = self.dets
        context["Pair_F"] = str(self.g1.currentText())
        context["Pair_B"] = str(self.g2.currentText())
        context["Pair_alpha"] = float(self.alpha.text())
        return context
