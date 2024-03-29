.. _ViewExercise1Solution:

========================
View Exercise 1 Solution
========================

main.py
#######

.. code-block:: python


    from qtpy import QtWidgets

    import sys

    import view

    """
    A wrapper class for setting the main window
    """
    class Demo(QtWidgets.QMainWindow):
        def __init__(self,parent=None):
            super().__init__(parent)

            self.window = QtWidgets.QMainWindow()
            my_view = view.View()

            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

    def get_qapplication_instance():
        if QtWidgets.QApplication.instance():
            app = QtWidgets.QApplication.instance()
        else:
            app = QtWidgets.QApplication(sys.argv)
        return app

    app = get_qapplication_instance()
    window = Demo()
    window.show()
    app.exec_()

view.py
#######

.. code-block:: python

    from qtpy import QtWidgets, QtCore, QtGui


    class View(QtWidgets.QWidget):

        def __init__(self, parent=None):
            super().__init__(parent)

            grid = QtWidgets.QVBoxLayout(self)

            self.table = QtWidgets.QTableWidget(self)
            self.table.setRowCount(4)
            self.table.setColumnCount(2)
            grid.addWidget(self.table)

            self.colours = QtWidgets.QComboBox()
            options = ["Blue", "Green", "Red"]
            self.colours.addItems(options)

            self.grid_lines = QtWidgets.QTableWidgetItem()
            self.grid_lines.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
            self.grid_lines.setCheckState(QtCore.Qt.Unchecked)
            self.addItemToTable("Show grid lines", self.grid_lines, 1)

            self.freq = QtWidgets.QTableWidgetItem("1.0")
            self.phi = QtWidgets.QTableWidgetItem("0.0")

            self.addWidgetToTable("Colour", self.colours, 0)
            self.addItemToTable("Frequency", self.freq, 2)
            self.addItemToTable("Phase", self.phi, 3)

            self.plot = QtWidgets.QPushButton('Add', self)
            self.plot.setStyleSheet("background-color:lightgrey")

            grid.addWidget(self.plot)

            self.setLayout(grid)

        def setTableRow(self, name, row):
            text = QtWidgets.QTableWidgetItem(name)
            text.setFlags(QtCore.Qt.ItemIsEnabled)
            col = 0
            self.table.setItem(row, col, text)

        def addWidgetToTable(self, name, widget, row):
            self.setTableRow(name,row)
            col = 1
            self.table.setCellWidget(row, col, widget)

        def addItemToTable(self, name, widget, row):
            self.setTableRow(name, row)
            col = 1
            self.table.setItem(row, col, widget)

In the above code the following functions have been added to prevent
repetition of code:

- ``setTableRow`` sets the label for the table row
- ``addWidgetToTable`` adds a widget to the table
- ``addItemToTable`` adds an item to the table (needed because the
  frequency and phase are items and not widgets)
