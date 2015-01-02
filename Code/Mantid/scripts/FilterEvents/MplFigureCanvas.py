from PyQt4 import QtGui

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

class Qt4MplCanvas(FigureCanvas):
    """
    """
    def __init__(self, parent):
        """ 
        """
        self.fig = Figure()
        self.axes = self.fig.add_subplot(111)
        # self.x = np.arange(0.0, 3.0, 0.01)
        # self.y = np.cos(2*np.pi*self.x)
        # self.axes.plot(self.x, self.y)

        FigureCanvas.__init__(self, self.fig)

        self.setParent(parent)

        FigureCanvas.setSizePolicy(self, QtGui.QSizePolicy.Expanding, 
                QtGui.QSizePolicy.Expanding)

        FigureCanvas.updateGeometry(self)

        return

    def plot(self, x, y):
        """
        """
        self.x = x
        self.y = y
        self.axes.plot(self.x, self.y)

        return

    def theplot(self):
        """
        """
        return self.axes
