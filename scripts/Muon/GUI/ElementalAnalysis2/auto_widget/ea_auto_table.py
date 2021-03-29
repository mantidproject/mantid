# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantid.simpleapi import AnalysisDataService

PEAK_TABLE_COLUMNS = ["centre" , "sigma" , "area"]


class EAAutoPopupTable(QtWidgets.QWidget):

    def __init__(self, table_name = "", is_find_peak = True, setup = False):
        super(EAAutoPopupTable , self).__init__(None)
        self.table = QtWidgets.QTableWidget(self)
        self.name = table_name
        if is_find_peak and setup:
            self.create_find_peak_table()
            self.populate_find_peak_table()
        elif not is_find_peak and  setup:
            self.create_match_table()
            self.populate_match_table()

        self.setup_example()
        self.setup_layout_interface()

    def setup_layout_interface(self):
        self.setObjectName("PopupTable")
        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.table)

        self.setLayout(self.vertical_layout)

    def setup_example(self):
        self.table.setColumnCount(3)
        self.table.setHorizontalHeaderLabels(["Peak Center" , "Sigma" , "Area"])
        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

        for i in range(3):
            self.table.horizontalHeaderItem(i).setToolTip("Example")

        self.add_entry_to_table(["30", "1", "600"])
        self.add_entry_to_table(["40", "1.5", "700"])
        self.add_entry_to_table(["50", "1.8", "800"])
        self.add_entry_to_table(["60", "2", "1000"])

    def create_find_peak_table(self):
        self.table.setColumnCount(3)
        self.table.setHorizontalHeaderLabels(["Peak Center", "Sigma", "Area"])

        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

    def create_match_table(self):
        table = AnalysisDataService.Instance().retrieve(self.name)
        columns = table.getColumnNames()

        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)

        header = self.table.horizontalHeader()
        for i in range(len(columns)):
            header.setSectionResizeMode(i, QtWidgets.QHeaderView.Stretch)

    def populate_find_peak_table(self):
        table = AnalysisDataService.Instance().retrieve(self.name)
        table_data = table.toDict()
        table_entries = []

        for i in range(table.rowCount()):
            table_entries.append([])
            for column in PEAK_TABLE_COLUMNS:
                table_entries[-1].append(table_data[column][i])

        for entry in table_entries:
            self.add_entry_to_table(entry)

    def populate_match_table(self):
        table = AnalysisDataService.Instance().retrieve(self.name)
        table_data = table.toDict()
        table_entries = []

        for i in range(table.rowCount()):
            table_entries.append([])
            for column in table_data:
                table_entries[-1].append(table_data[column][i])

        for entry in table_entries:
            self.add_entry_to_table(entry)

    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.table.columnCount()
        row_position = self.table.rowCount()
        self.table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            table_item = QtWidgets.QTableWidgetItem(entry)
            self.table.setItem(row_position, i, table_item)