from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtWidgets

from Muon.GUI.Common.utilities import table_utils


class RemovePlotWindow(QtWidgets.QDialog):
    applyRemoveSignal = QtCore.Signal(object)
    closeEventSignal = QtCore.Signal()

    def __init__(self, lines, subplot, vlines=[], parent=None):
        super(RemovePlotWindow, self).__init__()

        all_lines = lines + vlines
        self._subplot = subplot

        self.grid = QtWidgets.QGridLayout()

        self.table = QtWidgets.QTableWidget(self)
        self.table.resize(200, 200)

        self.table.setRowCount(len(all_lines))
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0, 150)
        self.table.setColumnWidth(1, 50)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(
            ("Line name;Remove").split(";"))
        table_utils.setTableHeaders(self.table)

        self.widgets = {}
        for index, line in enumerate(all_lines):
            table_utils.setRowName(self.table, index, line)
            tmp = {
                "line": line,
                "box": table_utils.addCheckBoxToTable(
                    self.table,
                    False,
                    index)}
            self.widgets[line] = tmp

        self.grid.addWidget(self.table)
        btn = QtWidgets.QPushButton("Remove")
        self.grid.addWidget(btn)
        self.setLayout(self.grid)
        self.setWindowTitle("Remove Lines For " + self._subplot)
        btn.clicked.connect(self.buttonClick)

    def closeEvent(self, event):
        self.closeEventSignal.emit()

    def buttonClick(self):
        self.applyRemoveSignal.emit(self.widgets.keys())

    def getState(self, name):
        return self.widgets[name]["box"].checkState() == QtCore.Qt.Checked

    def getLine(self, name):
        return self.widgets[name]["line"]

    @property
    def subplot(self):
        return self._subplot
