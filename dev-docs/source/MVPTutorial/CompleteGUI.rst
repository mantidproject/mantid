.. _CompleteGUI:

===========================
Complete GUI from Exercises
===========================

``main.py``
###########

.. code-block:: python

    from __future__ import (absolute_import,division,print_function)

    import PyQt4.QtGui as QtGui 
    import PyQt4.QtCore as QtCore

    import sys

    import model
    import masterView
    import masterPresenter

    
    """
    A wrapper class for setting the main window
    """
    class demo(QtGui.QMainWindow):
        def __init__(self, parent=None):
            super(demo,self).__init__(parent)
 
            data_model = model.DataGenerator()
            colour_model = model.ColourConvertor()

            self.window = QtGui.QMainWindow()

            my_view = masterView.MasterView(parent=self)
            self.master_presenter = masterPresenter.MasterPresenter(my_view, data_model, colour_model)

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

which has the addition of the data and colour models being passed to
the Presenter. This makes it easier for us to replace the Model at a
later date.

``masterView.py``
#################

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore

    import view
    import plotView

    import numpy as np

    class MasterView(QtGui.QWidget):

        def __init__(self, parent=None):
            super(MasterView, self).__init__(parent)

            grid = QtGui.QVBoxLayout(self)
            self.plot_view = plotView.PlotView()
            self.options_view=view.view()

            grid.addWidget(self.plot_view)          
            grid.addWidget(self.options_view)          

            self.setLayout(grid)

        def getOptionView(self):
            return self.options_view

        def getPlotView(self):
            return self.plot_view

``masterPresenter.py``
######################

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    import model
    import presenter
    import plotPresenter

    class MasterPresenter(object):

        def __init__(self, view, data_model, colour_model):
        self.view = view

        self.data_model = data_model
        self.colour_model = colour_model

        colours = self.colour_model.getColourSelection()

        self.presenter = presenter.Presenter(self.view.getOptionView(), colours)
        self.plot_presenter = plotPresenter.PlotPresenter(self.view.getPlotView())
        # connect statements
        self.view.getOptionView().plotSignal.connect(self.updatePlot)             
       
        # handle signals 
        def updatePlot(self):
            # only care about the colour if the button is pressed
            colour, freq,phi = self.presenter.getPlotInfo()
            grid_lines = self.presenter.getGridLines()
 
            self.data_model.genData(freq,phi )
            x_data = self.data_model.getXData()
            y_data = self.data_model.getYData()
 
            self.plot_presenter.plot(x_data, y_data, grid_lines, colour)

The signal from the View is caught here and the models are used to create the correct plot.

``plotPresenter.py``
####################

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    class PlotPresenter(object):

        def __init__(self, view):
            self.view = view

        def plot(self, x_data, y_data, grid_lines, colour_code):
            self.view.addData(x_data, y_data, grid_lines, colour_code, "x")

``plotView.py``
###############

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore
    import matplotlib.pyplot as plt

    from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas


    class PlotView(QtGui.QWidget):
        def __init__(self, parent=None):
            super(PlotView, self).__init__(parent)

            self.figure = plt.figure()
            grid = QtGui.QVBoxLayout(self)
            self.draw()
            self.canvas = self.getWidget()
            grid.addWidget(self.canvas)
            self.setLayout(grid)

        def draw(self):
            ax = self.figure.add_subplot(111)
            ax.clear()
            ax.set_xlim([0.0, 10.5])
            ax.set_ylim([-1.05, 1.05])
            ax.set_xlabel("time ($s$)")
            ax.set_ylabel("$f(t)$")
            return ax

        def getWidget(self):
            return FigureCanvas(self.figure)

        def addData(self, xvalues, yvalues, grid_lines, colour, marker):
            ax = self.draw()
            ax.grid(grid_lines)
            ax.plot(xvalues, yvalues, color=colour, marker=marker, linestyle="--")
            self.canvas.draw()

``presenter.py``
################

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)


    class Presenter(object):

        def __init__(self, view, colours):
            self.view = view
            self.view.setColours(colours)
       
        def getPlotInfo(self):
            return str(self.view.getColour()), self.view.getFreq(), self.view.getPhase()

        def getGridLines(self):
            return self.view.getGridLines()

``view.py``
###########

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import PyQt4.QtGui as QtGui
    import PyQt4.QtCore as QtCore


    class view(QtGui.QWidget):

        plotSignal = QtCore.pyqtSignal()

        def __init__(self, parent=None):
            super(view, self).__init__(parent)

            grid = QtGui.QVBoxLayout(self)

            self.table = QtGui.QTableWidget(self)
            self.table.setRowCount(4)
            self.table.setColumnCount(2)

            grid.addWidget(self.table)

            self.colours = QtGui.QComboBox()
            options=["Blue", "Green", "Red"]
            self.colours.addItems(options)

            self.grid_lines= QtGui.QTableWidgetItem()
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
            text = QtGui.QTableWidgetItem(name)
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

        def setColours(self, options):
            self.colours.clear()
            self.colours.addItems(options)

``model.py``
############

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)
    import numpy as np

    class DataGenerator(object):

        def __init__(self):
            self.x_data = np.linspace(0.0, 10.0, 100)
            self.y_data = []

        def genData(self, freq, phi):
            self.y_data = np.sin(freq * self.x_data + phi)

        def getXData(self):
            return self.x_data

        def getYData(self):
            return self.y_data


    class ColourConverter(object):

        def __init__(self):
            self.colour_table = {"red": "r", "blue": "b", "black": "k"}

        def getColourSelection(self):
            return self.colour_table.keys()

