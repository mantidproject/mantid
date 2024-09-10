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


    def get_qapplication_instance():
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)

    app = get_qapplication_instance()
    window = View()
    window.show()
    app.exec_()

view.py
#######

.. code-block:: python

    from qtpy.QtCore.Qt import ItemIsEnabled, ItemIsUserCheckable, Unchecked
    from qtpy.QtWidgets import QComboBox, QPushButton, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget

    from typing import Union


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
            self._grid_lines.setFlags(ItemIsUserCheckable | ItemIsEnabled)
            self._grid_lines.setCheckState(Unchecked)
            self.addItemToTable("Show grid lines", self._grid_lines, 1)

            freq = QTableWidgetItem("1.0")
            phi = QTableWidgetItem("0.0")

            self.addWidgetToTable("Colour", self._colours, 0)
            self.addItemToTable("Frequency", freq, 2)
            self.addItemToTable("Phase", phi, 3)

            self._plot = QPushButton('Add', self)
            self._plot.setStyleSheet("background-color:lightgrey")

            grid.addWidget(self._plot)

            self.setLayout(grid)

        def setTableRow(self, name: str, row: int) -> None:
            text = QTableWidgetItem(name)
            text.setFlags(ItemIsEnabled)
            col = 0
            self._table.setItem(row, col, text)

        def addWidgetToTable(self, name: str, widget: QWidget, row: int) -> None:
            self.setTableRow(name,row)
            col = 1
            self._table.setCellWidget(row, col, widget)

        def addItemToTable(self, name: str, widget: QWidget, row: int) -> None:
            self.setTableRow(name, row)
            col = 1
            self._table.setItem(row, col, widget)

In the above code the following functions have been added to prevent
repetition of code:

- ``setTableRow`` sets the label for the table row
- ``addWidgetToTable`` adds a widget to the table
- ``addItemToTable`` adds an item to the table (needed because the
  frequency and phase are items and not widgets)
