# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.utilities import table_utils

HEADERS = ["Period Number", "Name", "Type", "DAQ Number", "Frames", "Total Good Frames", "Counts"]
HEADER_STYLE = "QHeaderView { font-weight: bold; }"
COLUMN_COUNT = 7
HEADER_COLUMN_MAP = {"Period Number": 0,
                     "Name": 1,
                     "Type": 2,
                     "DAQ Number": 3,
                     "Frames": 4,
                     "Total Good Frames": 5,
                     "Counts": 6}
CONTEXT_MAP = {"Name": 1,
               "Type": 2,
               "Frames": 3,
               "Total Good Frames": 4,
               "Counts": 5}
PERIOD_INFO_NOT_FOUND = "Not found"
NOT_DAQ_STRING = "-"
DAQ = "1"
DWELL = "2"
CYCLES_NOT_FOUND = "Number of period cycles not found"
INFO_DELIM = ';'


class MuonPeriodInfoWidget(QtWidgets.QWidget):
    """
    This is a simple widget used by the Muon GUI's to display additional information about periods of a run.

    - Number of sequences is a string value
    """

    def __init__(self, parent=None):
        super(MuonPeriodInfoWidget, self).__init__(parent)

        self._label = None
        self._table = None
        self._number_of_sequences = None
        self._daq_count = 0

        # Create layout
        self._create_layout()

    @property
    def number_of_sequences(self):
        return self._number_of_sequences

    @number_of_sequences.setter
    def number_of_sequences(self, value: str):
        self._number_of_sequences = value
        if self._label:
            if value:
                self._label.setText("Run contains " + value + " cycles of periods")
            else:
                self._label.setText(CYCLES_NOT_FOUND)

    @property
    def daq_count(self):
        return self._daq_count

    def add_period_to_table(self, name=PERIOD_INFO_NOT_FOUND, period_type=PERIOD_INFO_NOT_FOUND,
                            frames=PERIOD_INFO_NOT_FOUND, total_frames=PERIOD_INFO_NOT_FOUND,
                            counts=PERIOD_INFO_NOT_FOUND):
        row_num = self._num_rows()
        self._table.insertRow(row_num)
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Period Number"], self._new_text_widget(str(row_num + 1)))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Name"], self._new_text_widget(name))
        if period_type == DAQ:
            self._daq_count += 1
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Type"], self._new_text_widget("DAQ"))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["DAQ Number"], self._new_text_widget(str(self._daq_count)))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Counts"], self._new_text_widget(counts))
        elif period_type == DWELL:
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Type"], self._new_text_widget("DWELL"))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["DAQ Number"], self._new_text_widget(NOT_DAQ_STRING))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Counts"], self._new_text_widget(NOT_DAQ_STRING))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Frames"], self._new_text_widget(frames))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Total Good Frames"], self._new_text_widget(total_frames))

    def is_empty(self):
        if self._num_rows() > 0:
            return False
        return True

    def clear(self):
        self._daq_count = 0
        self.number_of_sequences = None  # Use setter here to reset label
        for row in reversed(range(self._num_rows())):
            self._table.removeRow(row)

    def _new_text_widget(self, text):
        new_widget = table_utils.ValidatedTableItem(lambda text: True)
        new_widget.setText(text)
        new_widget.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
        return new_widget

    def _num_rows(self):
        return self._table.rowCount()

    def _create_layout(self):
        self._label = QtWidgets.QLabel(CYCLES_NOT_FOUND)
        self._table = QtWidgets.QTableWidget(0, COLUMN_COUNT, parent=self)
        self._table.setHorizontalHeaderLabels(HEADERS)
        self._table.horizontalHeader().setStyleSheet(HEADER_STYLE)
        self._table.verticalHeader().setVisible(False)
        header = self._table.horizontalHeader()
        for i in range(COLUMN_COUNT):
            header.setSectionResizeMode(i, QtWidgets.QHeaderView.ResizeToContents)
        self.layout = QtWidgets.QVBoxLayout()
        self.layout.addWidget(self._label)
        self.layout.addWidget(self._table)
        self.setLayout(self.layout)
        self.setWindowTitle("Period Information")
