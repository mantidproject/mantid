# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, PYQT4, QtCore
from Muon.GUI.Common.utilities import table_utils

HEADERS = ["Period Number", "Name", "Total Frames"]
HEADER_STYLE = "QHeaderView { font-weight: bold; }"
COLUMN_COUNT = 3


class MuonPeriodInfoWidget(QtWidgets.QWidget):
    """
    This is a simple widget used by the Muon GUI's to display additional information about periods of a run.
    """

    def __init__(self, parent=None):
        super(MuonPeriodInfoWidget, self).__init__(parent)

        self._label = None
        self._table = None
        self._number_of_sequencs = 0

        # Create layout
        self._create_layout()

    @property
    def number_of_sequences(self):
        return self._number_of_sequences

    @number_of_sequences.setter
    def number_of_sequences(self, value):
        self._number_of_sequences = value
        if self.label:
            if value:
                self.label.setText("Run contains " + value + " cycles of periods")
            else:
                self.label.setText("Number of period cycles not found")

    def add_period_to_table(self, name, total_frames):
        row_num = self._num_rows()
        self.table.insertRow(row_num)
        self.table.setItem(row_num, 0, self._new_text_widget(str(row_num + 1)))  # Set period number
        self.table.setItem(row_num, 1, self._new_text_widget(name))  # Set name
        self.table.setItem(row_num, 2, self._new_text_widget(total_frames))  # Set name

    def _new_text_widget(self, text):
        new_widget = table_utils.ValidatedTableItem(lambda text: True)
        new_widget.setText(text)
        new_widget.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
        return new_widget

    def _num_rows(self):
        return self.table.rowCount()

    def _create_layout(self):
        self.label = QtWidgets.QLabel("Run contains 0 cycles of periods")
        self.table = QtWidgets.QTableWidget(0, COLUMN_COUNT, parent=self)
        self.table.setHorizontalHeaderLabels(HEADERS)
        self.table.horizontalHeader().setStyleSheet(HEADER_STYLE)
        self.table.verticalHeader().setVisible(False)
        self.layout = QtWidgets.QVBoxLayout()
        self.layout.addWidget(self.label)
        self.layout.addWidget(self.table)
        self.setLayout(self.layout)
        self.setWindowTitle("Period Information")
