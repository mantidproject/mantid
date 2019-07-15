.. _PresenterExerciseSolution:

===========================
Presenter Exercise Solution
===========================

View
####

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    from qtpy import QtWidgets, QtCore, QtGui


    class View(QtWidgets.QWidget):

        plotSignal = QtCore.Signal()

        def __init__(self, parent=None):
            super(view, self).__init__(parent)

            grid = QtWidgets.QVBoxLayout(self)

            self.table = QtWidgets.QTableWidget(self)
            self.table.setRowCount(4)
            self.table.setColumnCount(2)
           
            grid.addWidget(self.table)           

            self.colours = QtWidgets.QComboBox()
            options=["Blue", "Green", "Red"]
            self.colours.addItems(options)

            self.grid_lines= QtWidgets.QTableWidgetItem()
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

            self.plot.clicked.connect(self.buttonPressed)

        def getColour(self):
            return self.colours.currentText()
  
        def getGridLines(self):
            return self.grid_lines.checkState() == QtCore.Qt.Checked

        def getFreq(self):
            return float(self.freq.text())

        def getPhase(self):
            return float(self.phi.text())

        def buttonPressed(self):
            self.plotSignal.emit()

        def setTableRow(self, name, row):
            text = QtWidgets.QTableWidgetItem(name)
            text.setFlags(QtCore.Qt.ItemIsEnabled)
            col = 0
            self.table.setItem(row, col, text)

        def addWidgetToTable(self, name, widget, row):
            self.setTableRow(name, row)
            col = 1
            self.table.setCellWidget(row, col, widget)
        
        def addItemToTable(self, name, widget, row):
            self.setTableRow(name, row)
            col = 1
            self.table.setItem(row, col, widget)
    
Presenter
#########

.. code-block:: python

    from __future__ import (absolute_import ,division, print_function)

    class Presenter(object):

        # pass the view and model into the presenter
        def __init__(self, view):
            self.view = view

            self.view.plotSignal.connect(self.updatePlot)             
       
        # handle signals 
        def updatePlot(self):
            print("The table settings are:")
            print("   colour     : " + str(self.view.getColour()))
            print("   Grid lines : " + str(self.view.getGridLines()))
            print("   Frequency  : " + str(self.view.getFreq()))
            print("   Phase      : " + str(self.view.getPhase()))

Main module
###########

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    from qtpy import QtWidgets, QtCore, QtGui

    import sys

    import view
    import presenter


    """
    A wrapper class for setting the main window
    """
    class Demo(QtWidgets.QMainWindow):
        def __init__(self, parent=None):
            super(Demo,self).__init__(parent)

            self.window = QtWidgets.QMainWindow()
            my_view = view.View()
            self.presenter = presenter.Presenter(my_view)
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
