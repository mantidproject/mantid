.. _ViewExercise1Solution:

========================
View Exercise 1 Solution
========================

main.py
#######

.. code-block:: python

    from __future__ import (absolute_import,division,print_function)

    import PyQt4.QtGui as QtGui 
    import PyQt4.QtCore as QtCore

    import sys

    import view

    """
    A wrapper class for setting the main window
    """
    class demo(QtGui.QMainWindow):
        def __init__(self,parent=None):
            super(demo,self).__init__(parent)

            self.window = QtGui.QMainWindow()
            my_view = view.view()

            # set the view for the main window
            self.setCentralWidget(my_view)
            self.setWindowTitle("view tutorial")

        def qapp():
            if QtGui.QApplication.instance():
                _app = QtGui.QApplication.instance()
            else:
                _app = QtGui.QApplication(sys.argv)
            return _app

    app = qapp()
    window = demo()
    window.show()
    app.exec_()

view.py
#######

.. code-block:: python

    from __future__ import (absolute_import,division,print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore


    class view(QtGui.QWidget):

        def __init__(self, parent=None):
            super(view, self).__init__(parent)

            grid = QtGui.QVBoxLayout(self)

            self.table = QtGui.QTableWidget(self)
            self.table.setRowCount(4)
            self.table.setColumnCount(2)
            grid.addWidget(self.table)           

            self.colours = QtGui.QComboBox()
            options = ["Blue", "Green", "Red"]
            self.colours.addItems(options)

            self.grid_lines = QtGui.QTableWidgetItem()
            self.grid_lines.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
            self.grid_lines.setCheckState(QtCore.Qt.Unchecked)
            self.addItemToTable("Show grid lines", self.grid_lines, 1)

            self.freq = QtGui.QTableWidgetItem("1.0")
            self.phi = QtGui.QTableWidgetItem("0.0")

            self.addWidgetToTable("Colour", self.colours, 0)
            self.addItemToTable("Frequency", self.freq, 2)
            self.addItemToTable("Phase", self.phi, 3)

            self.plot = QtGui.QPushButton('Add', self)
            self.plot.setStyleSheet("background-color:lightgrey")

            grid.addWidget(self.plot)           

            self.setLayout(grid)

     def setTableRow(self, name, row):
            text = QtGui.QTableWidgetItem(name)
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
