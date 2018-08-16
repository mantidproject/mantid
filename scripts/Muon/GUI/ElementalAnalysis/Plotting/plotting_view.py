from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils

#from mantid import plots

from collections import OrderedDict

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from PyQt4 import QtGui

from random import choice, randint
from time import time


class PlotView(QtGui.QWidget):
    def __init__(self):
        super(PlotView, self).__init__()
        self.plots = OrderedDict({})
        self.current_grid = None
        self.gridspecs = {
            1: gridspec.GridSpec(1, 1),
            2: gridspec.GridSpec(1, 2),
            3: gridspec.GridSpec(3, 1),
            4: gridspec.GridSpec(2, 2)
        }
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        self.last_positions = []

        self.plot_selector = QtGui.QComboBox()

        button_layout = QtGui.QHBoxLayout()
        self.x_axis_changer = putils.AxisChanger("X")
        self.y_axis_changer = putils.AxisChanger("Y")

        ### to test functionality: ###
        self.add_button = QtGui.QPushButton("Add")
        self.del_button = QtGui.QPushButton("Del")

        def _add(): return self.add_subplot("{:.2f}".format(time()))

        def _del(): return self.remove_subplot(choice(self.plots.keys()))
        self.add_button.clicked.connect(_add)
        self.del_button.clicked.connect(_del)

        button_layout.addWidget(self.add_button)
        button_layout.addWidget(self.del_button)
        ###  -------------------  ###

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer)
        button_layout.addWidget(self.y_axis_changer)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.canvas, 0, 0)
        grid.addLayout(button_layout, 1, 0)
        self.setLayout(grid)

    def _get_positions(self):
        return [p.get_position() for p in self.plots.values()]

    def _set_positions(self, positions):
        for plot, pos in zip(self.plots.values(), positions):
            plot.set_position(
                self.current_grid[pos[0], pos[1]].get_position(self.figure))

    def _update_gridspec(self, new_plots, last=None):
        if new_plots:
            self.current_grid = self.gridspecs[new_plots]
            positions = putils.get_layout(new_plots)
            self._set_positions(positions)
            if last is not None:
                # label is necessary to fix
                # https://github.com/matplotlib/matplotlib/issues/4786
                self.plots[last] = self.figure.add_subplot(
                    self.current_grid[positions[-1][0], positions[-1][1]], label=last)
                # testing purposes
                self.plots[last].plot([randint(0, 100) for i in range(5)])
                self.plots[last].set_title(last)
        self.canvas.draw()
        self.update_plot_selector()

    def update_plot_selector(self):
        self.plot_selector.clear()
        self.plot_selector.addItems(self.plots.keys())

    def add_subplot(self, name):
        """ will raise KeyError if: plots exceed 4 """
        self._update_gridspec(len(self.plots) + 1, last=name)
        return self.plots[name]

    def remove_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; there are no plots """
        self.figure.delaxes(self.plots[name])
        self.canvas.draw()
        del self.plots[name]
        self._update_gridspec(len(self.plots))
