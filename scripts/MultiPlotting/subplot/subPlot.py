from __future__ import (absolute_import, division, print_function)
from six import iteritems

from mantid import plots
from qtpy import QtWidgets, QtCore

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from MultiPlotting.navigation_toolbar import myToolbar

# use this to manage lines and workspaces directly

# visualises multiple plots
class subPlot(QtWidgets.QWidget):

    def __init__(self, name, context):
        super(subPlot,self).__init__()
        self._context = context
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        grid = QtWidgets.QGridLayout()

        # add toolbar
        self.toolbar = myToolbar(self.canvas, self)
        self.toolbar.update()
        grid.addWidget(self.toolbar, 0, 0)

        # add plot
        self.plotObjects = {}

        self.plotObjects["test"] = self.figure.add_subplot(111)
        print(self.plotObjects["test"],self.figure)

        grid.addWidget(self.canvas, 1, 0)
        self.setLayout(grid)

    def _add_plotted_line(self, subplotName, label, lines, workspace):
        """ Appends plotted lines to the related subplot list. """
        self._context.addLine(subplotName,label, lines, workspace)
        self.canvas.draw()

    def plot(self, subplotName, label, workspace):
        line, = plots.plotfunctions.plot(self.plotObjects[subplotName],workspace,specNum=1)
        self._add_plotted_line(subplotName,line.get_label(), [line], workspace)