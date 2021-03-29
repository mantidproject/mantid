# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtGui, QtCore
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable
from Muon.GUI.Common.message_box import warning

group_table_columns = {0: 'Element', 1: 'Likelihood', 2: 'Count' }
inverse_group_table_columns = {'Element': 0, 'Likelihood': 1, 'Count': 2}
group_table_column_tooltip = {0: "Element symbol" ,
                              1: "A weigthed count of element occurences with those with less error"
                                 " having a larger weight ",
                              2: "Number of Occurences of element"}

class EAMatchTableView(QtWidgets.QWidget):

    def __init__(self,parent = None):
        super(EAMatchTableView, self).__init__(parent)
        self.table = QtWidgets.QTableWidget(self)
        self.setup_table()
        self.setup_interface_layout()
        self.add_entry_to_table(["Ag","14" ,"6"])
        self.add_entry_to_table(["Au", "12", "5"])
        self.add_entry_to_table(["Fe", "10", "4"])
        self.add_entry_to_table(["Li", "8", "3"])

    def setup_table(self):
        self.table.setColumnCount(3)
        self.table.setHorizontalHeaderLabels(list(inverse_group_table_columns))

        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

        for column_index,tooltip in group_table_column_tooltip.items():
            self.table.horizontalHeaderItem(column_index).setToolTip(tooltip)

    def setup_interface_layout(self):
        self.setObjectName("MatchTableView")
        self.resize(200, 500)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.table)

        self.setLayout(self.vertical_layout)

    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.table.columnCount()
        row_position = self.table.rowCount()
        self.table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            table_item = QtWidgets.QTableWidgetItem(str(entry))
            self.table.setItem(row_position, i, table_item)