# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

GROUP_TABLE_COLUMNS = {0: 'Run', 1: 'Detector', 2: 'Probable elements'}
INVERSE_GROUP_TABLE_COLUMNS = {'Run': 0, 'Detector': 1, 'Probable elements': 2}
GROUP_TABLE_COLUMN_TOOLTIP = {0: "Run number",
                              1: "Detector number",
                              2: "3 most probable elements"}


class EAMatchTableView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAMatchTableView, self).__init__(parent)
        self.table = QtWidgets.QTableWidget(self)
        self.setup_table()
        self.setup_interface_layout()

    def setup_table(self):
        self.table.setColumnCount(3)
        self.table.setHorizontalHeaderLabels(list(INVERSE_GROUP_TABLE_COLUMNS))

        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

        for column_index, tooltip in GROUP_TABLE_COLUMN_TOOLTIP.items():
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

    def remove_row(self, row_index):
        self.table.removeRow(row_index)

    def clear_table(self):
        for i in range(self.table.rowCount() - 1, -1, -1):
            self.remove_row(i)
