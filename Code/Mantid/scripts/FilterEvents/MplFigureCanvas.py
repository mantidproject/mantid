from PyQt4 import QtGui

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

class Qt4MplCanvas(FigureCanvas):
    """  A customized Qt widget for matplotlib figure. 
    It can be used to replace GraphicsView of QtGui
    """
    def __init__(self, parent):
        """  Initialization
        """
        # Instantialize matplotlib Figure
        self.fig = Figure()
        self.axes = self.fig.add_subplot(111)

        # Initialize parent class and set parent
        FigureCanvas.__init__(self, self.fig)
        self.setParent(parent)

        # Set size policy to be able to expanding and resizable with frame
        FigureCanvas.setSizePolicy(self, QtGui.QSizePolicy.Expanding, 
                QtGui.QSizePolicy.Expanding)

        FigureCanvas.updateGeometry(self)

        return

    def plot(self, x, y):
        """ Plot a set of data 
        Argument:
        - x: numpy array X
        - y: numpy array Y
        """
        self.x = x
        self.y = y
        self.axes.plot(self.x, self.y)

        return

    def getPlot(self):
        """ reture figure's axes to expose the matplotlib figure to PyQt client 
        """
        return self.axes
