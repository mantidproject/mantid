.. _ViewExercise1Solution:

========================
View Exercise 1 Solution
========================

main.py
#######

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from view import View


    def _get_qapplication_instance():
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)

    if __name__ == "__main__" :
        app = _get_qapplication_instance()
        window = View()
        window.show()
        app.exec_()

view.py
#######

.. code-block:: python

    from qtpy.QtCore import Qt
    from qtpy.QtWidgets import QComboBox, QPushButton, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget

    from typing import Union

    TEXT_COLUMN = 0
    WIDGET_COLUMN = 1


    class View(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)
            self.setWindowTitle("view tutorial")

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

            freq = QTableWidgetItem("1.0")
            phi = QTableWidgetItem("0.0")

            self._add_widget_to_table("Colour", self._colours, 0)
            self._add_item_to_table("Frequency", freq, 2)
            self._add_item_to_table("Phase", phi, 3)

            self._plot = QPushButton('Add', self)
            self._plot.setStyleSheet("background-color:lightgrey")

            grid.addWidget(self._plot)

            self.setLayout(grid)

        def _set_table_row(self, name: str, row: int) -> None:
            text = QTableWidgetItem(name)
            text.setFlags(Qt.ItemIsEnabled)
            self._table.setItem(row, TEXT_COLUMN, text)

        def _add_widget_to_table(self, name: str, widget: QWidget, row: int) -> None:
            self._set_table_row(name,row)
            self._table.setCellWidget(row, WIDGET_COLUMN, widget)

        def _add_item_to_table(self, name: str, widget: QWidget, row: int) -> None:
            self._set_table_row(name, row)
            self._table.setItem(row, WIDGET_COLUMN, widget)

In the above code the following functions have been added to prevent
repetition of code:

- ``_set_table_row`` sets the label for the table row
- ``_add_widget_to_table`` adds a widget to the table
- ``_add_item_to_table`` adds an item to the table (needed because the
  frequency and phase are items and not widgets)
