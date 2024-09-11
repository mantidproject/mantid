# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore.Qt import ItemIsEnabled
from qtpy.QtWidgets import QComboBox, QGridLayout, QPushButton, QTableWidget, QTableWidgetItem, QWidget
from typing import List, Union


class View(QWidget):

    def __init__(self, parent: Union[QWidget, None] = None):
        super(View, self).__init__(parent)

        self._presenter = None

        self._table = QTableWidget()
        self._table.setWindowTitle("MVP Demo")
        self._table.resize(400, 250)
        self._table.setRowCount(5)
        self._table.setColumnCount(2)
        self._table.setHorizontalHeaderLabels("name;value;".split(";"))

        # Set display values in the widgets
        keys = ["value 1", "operation", "value 2", "display", "result"]
        self._combo = {}
        self._create_combo_table(1, 1, "operations")
        self._create_combo_table(3, 1, "display")
        for row in range(len(keys)):
            self._set_names(keys[row], row)

        grid = QGridLayout()
        grid.addWidget(self._table)

        self._button = QPushButton("Calculate", self)
        self._button.setStyleSheet("background-color:lightgrey")
        grid.addWidget(self._button)

        self._button.clicked.connect(self._button_clicked)
        self._combo["display"].currentIndexChanged.connect(self._display_changed)

        self.setLayout(grid)

    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter

    def _button_clicked(self) -> None:
        self._presenter.handle_button_clicked()

    def _display_changed(self) -> None:
        self._presenter.handle_display_changed()

    def _create_combo_table(self, row: int, col: int, key: str) -> None:
        self._combo[key] = QComboBox()
        options = ["test"]
        self._combo[key].addItems(options)
        self._table.setCellWidget(row, col, self._combo[key])

    def _set_names(self, name: str, row: int) -> None:
        text = QTableWidgetItem(name)
        text.setFlags(ItemIsEnabled)
        self._table.setItem(row, 0, text)

    def set_options(self, key: str, options: List[str]) -> None:
        self._combo[key].clear()
        self._combo[key].addItems(options)

    def set_result(self, value: float) -> None:
        self._table.setItem(4, 1, QTableWidgetItem(str(value)))

    def show_display(self, show: bool) -> None:
        self._table.setRowHidden(4, show)

    def get_value(self, row: int) -> float:
        return float(self._table.item(row, 1).text())

    def get_operation(self) -> str:
        return self._combo["operations"].currentText()

    def get_display(self) -> str:
        return self._combo["display"].currentText()
