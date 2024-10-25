# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QComboBox, QPushButton, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget
from typing import List, Union

TEXT_COLUMN = 0
WIDGET_COLUMN = 1


class View(QWidget):
    def __init__(self, parent: Union[QWidget, None] = None):
        super().__init__(parent)

        self._presenter = None

        grid = QVBoxLayout(self)

        self._table = QTableWidget(self)
        self._table.setRowCount(4)
        self._table.setColumnCount(2)

        grid.addWidget(self._table)

        self._colours = QComboBox()
        options = ["Blue", "Green", "Red"]
        self._colours.addItems(options)

        self._grid_lines = QTableWidgetItem()
        self._grid_lines.setFlags(Qt.ItemIsUserCheckable | Qt.ItemIsEnabled)
        self._grid_lines.setCheckState(Qt.Unchecked)
        self._add_item_to_table("Show grid lines", self._grid_lines, 1)

        self._freq = QTableWidgetItem("1.0")
        self._phi = QTableWidgetItem("0.0")

        self._add_widget_to_table("Colour", self._colours, 0)
        self._add_item_to_table("Frequency", self._freq, 2)
        self._add_item_to_table("Phase", self._phi, 3)

        self._plot = QPushButton("Add", self)
        self._plot.setStyleSheet("background-color:lightgrey")

        grid.addWidget(self._plot)

        self.setLayout(grid)

        self._plot.clicked.connect(self._button_clicked)

    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter

    def get_colour(self) -> str:
        return self._colours.currentText()

    def get_grid_lines(self) -> bool:
        return self._grid_lines.checkState() == Qt.Checked

    def get_freq(self) -> float:
        return float(self._freq.text())

    def get_phase(self) -> float:
        return float(self._phi.text())

    def _button_clicked(self):
        self._presenter.handle_update_plot()

    def _set_table_row(self, name: str, row: int) -> None:
        text = QTableWidgetItem(name)
        text.setFlags(Qt.ItemIsEnabled)
        self._table.setItem(row, TEXT_COLUMN, text)

    def _add_widget_to_table(self, name: str, widget: QWidget, row: int) -> None:
        self._set_table_row(name, row)
        self._table.setCellWidget(row, WIDGET_COLUMN, widget)

    def _add_item_to_table(self, name: str, widget: QWidget, row: int) -> None:
        self._set_table_row(name, row)
        self._table.setItem(row, WIDGET_COLUMN, widget)

    def set_colours(self, options: List[str]) -> None:
        self._colours.clear()
        self._colours.addItems(options)
