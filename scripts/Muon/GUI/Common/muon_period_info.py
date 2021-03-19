# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, PYQT4, QtCore
from Muon.GUI.Common.utilities import table_utils

HEADERS = ["Period Number", "Name", "Type", "DAQ Number", "Frames", "Total Frames"]
HEADER_STYLE = "QHeaderView { font-weight: bold; }"
COLUMN_COUNT = 6
HEADER_COLUMN_MAP = {"Period Number": 0,
                     "Name": 1,
                     "Type": 2,
                     "DAQ Number": 3,
                     "Frames": 4,
                     "Total Frames": 5}
CONTEXT_MAP = {"Name": 1,
               "Type": 2,
               "Frames": 3,
               "Total Frames": 4}
PERIOD_INFO_NOT_FOUND = "Not found"
DWELL_STRING = "-"
DAQ = "1"
DWELL = "2"


class MuonPeriodInfoWidget(QtWidgets.QWidget):
    """
    This is a simple widget used by the Muon GUI's to display additional information about periods of a run.
    """

    def __init__(self, parent=None):
        super(MuonPeriodInfoWidget, self).__init__(parent)

        self._label = None
        self._table = None
        self._number_of_sequences = 0
        self._daq_count = 0

        # Create layout
        self._create_layout()

    @property
    def number_of_sequences(self):
        return self._number_of_sequences

    @number_of_sequences.setter
    def number_of_sequences(self, value):
        self._number_of_sequences = value
        if self._label:
            if value:
                self._label.setText("Run contains " + value + " cycles of periods")
            else:
                self._label.setText("Number of period cycles not found")

    def add_period_to_table(self, name, type, frames, total_frames):
        row_num = self._num_rows()
        self._table.insertRow(row_num)
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Period Number"], self._new_text_widget(str(row_num + 1)))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Name"], self._new_text_widget(name))
        if type == DAQ:
            self._daq_count += 1
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Type"], self._new_text_widget("DAQ"))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["DAQ Number"], self._new_text_widget(str(self._daq_count)))
        elif type == DWELL:
            self._table.setItem(row_num, HEADER_COLUMN_MAP["Type"], self._new_text_widget("DWELL"))
            self._table.setItem(row_num, HEADER_COLUMN_MAP["DAQ Number"], self._new_text_widget(DWELL_STRING))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Frames"], self._new_text_widget(frames))
        self._table.setItem(row_num, HEADER_COLUMN_MAP["Total Frames"], self._new_text_widget(total_frames))

    def is_empty(self):
        if self._num_rows() > 0:
            return False
        return True

    def clear(self):
        self._daq_count = 0
        self.number_of_sequences = 0 # Use setter here to reset label
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
        self._label = QtWidgets.QLabel("Run contains 0 cycles of periods")
        self._table = QtWidgets.QTableWidget(0, COLUMN_COUNT, parent=self)
        self._table.setHorizontalHeaderLabels(HEADERS)
        self._table.horizontalHeader().setStyleSheet(HEADER_STYLE)
        self._table.verticalHeader().setVisible(False)
        self.layout = QtWidgets.QVBoxLayout()
        self.layout.addWidget(self._label)
        self.layout.addWidget(self._table)
        self.setLayout(self.layout)
        self.setWindowTitle("Period Information")
