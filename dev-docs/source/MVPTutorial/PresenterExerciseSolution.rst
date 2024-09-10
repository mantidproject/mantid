.. _PresenterExerciseSolution:

===========================
Presenter Exercise Solution
===========================

The View
########

.. code-block:: python

    from qtpy.QtCore import Checked, ItemIsEnabled, ItemIsUserCheckable, Unchecked
    from qtpy.QtWidgets import QComboBox, QPushButton, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget
    from typing import Union

    TEXT_COLUMN = 0
    WIDGET_COLUMN = 1


    class View(QWidget):

        def __init__(self, parent: Union[QWidget, None]=None):
            super().__init__(parent)

            self._presenter = None

            grid = QVBoxLayout(self)

            self._table = QTableWidget(self)
            self._table.setRowCount(4)
            self._table.setColumnCount(2)

            grid.addWidget(self._table)

            self._colours = QComboBox()
            options=["Blue", "Green", "Red"]
            self._colours.addItems(options)

            self._grid_lines= QTableWidgetItem()
            self._grid_lines.setFlags(ItemIsUserCheckable | ItemIsEnabled)
            self._grid_lines.setCheckState(Unchecked)
            self._add_item_to_table("Show grid lines", self._grid_lines, 1)

            self._freq = QTableWidgetItem("1.0")
            self._phi = QTableWidgetItem("0.0")

            self._add_widget_to_table("Colour", self._colours, 0)
            self._add_item_to_table("Frequency", self._freq, 2)
            self._add_item_to_table("Phase", self._phi, 3)

            self._plot = QPushButton('Add', self)
            self._plot.setStyleSheet("background-color:lightgrey")

            grid.addWidget(self._plot)

            self.setLayout(grid)

            self._plot.clicked.connect(self._button_clicked)

        def get_colour(self) -> str:
            return self._colours.currentText()

        def get_grid_lines(self) -> bool:
            return self._grid_lines.checkState() == Checked

        def get_freq(self) -> float:
            return float(self._freq.text())

        def get_phase(self) -> float:
            return float(self._phi.text())

        def _button_clicked(self):
            self._presenter.handle_plot_clicked()

        def _set_table_row(self, name: str, row: int) -> None:
            text = QTableWidgetItem(name)
            text.setFlags(ItemIsEnabled)
            self._table.setItem(row, TEXT_COLUMN, text)

        def _add_widget_to_table(self, name: str, widget: QWidget, row: int) -> None:
            self._set_table_row(name, row)
            self._table.setCellWidget(row, WIDGET_COLUMN, widget)

        def _add_item_to_table(self, name: str, widget: QWidget, row: int) -> None:
            self._set_table_row(name, row)
            self._table.setItem(row, WIDGET_COLUMN, widget)

The Presenter
#############

.. code-block:: python

    class Presenter:

        def __init__(self, view):
            self._view = view
            self._view.subscribe_presenter(self)

        def handle_plot_clicked(self) -> None:
            print("The table settings are:")
            print(f"   colour     : {self._view.get_colour()}")
            print(f"   Grid lines : {self._view.get_grid_lines()}")
            print(f"   Frequency  : {self._view.get_freq()}")
            print(f"   Phase      : {self._view.get_phase()}")

The Main
########

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QApplication

    from view import View
    from presenter import Presenter


    def _get_qapplication_instance() -> QApplication:
        if app := QApplication.instance():
            return app
        return QApplication(sys.argv)


    app = _get_qapplication_instance()
    view = View()
    presenter = Presenter(view)
    view.show()
    app.exec_()
