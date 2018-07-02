===========================
Complete GUI from Exercises
===========================

Main module
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

Master View
###########

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

Master Presenter
################

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

Plot Presenter
##############

.. code-block:: python

    from __future__ import (absolute_import, division, print_function)

    class PlotPresenter(object):

        def __init__(self, view):
            self.view = view

	def plot(self, x_data, y_data, grid_lines, colour_code):
            self.view.addData(x_data, y_data, grid_lines, colour_code, "x")

PlotView
########

Unchanged from `Matplotlib and MVP <Matplotlib.html>`_.

Presenter
#########

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

View
####

Unchanged from `Model Exercise Solution <ModelExerciseSolution.html>`_.

Model
#####

Unchanged from `Model Exercise Solution <ModelExerciseSolution.html>`_.
