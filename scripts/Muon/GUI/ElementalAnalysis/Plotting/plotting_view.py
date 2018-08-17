from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_view import AxisChangerView


#from mantid import plots

from collections import OrderedDict

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from PyQt4 import QtGui


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

        self.plot_selector = QtGui.QComboBox()

        button_layout = QtGui.QHBoxLayout()
        self.x_axis_changer = AxisChangerPresenter(AxisChangerView("X"))
        self.y_axis_changer = AxisChangerPresenter(AxisChangerView("Y"))

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.y_axis_changer.view)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.canvas, 0, 0)
        grid.addLayout(button_layout, 1, 0)
        self.setLayout(grid)

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
        self.canvas.draw()
        self._update_plot_selector()

    def _update_plot_selector(self):
        self.plot_selector.clear()
        self.plot_selector.addItems(self.plots.keys())

    def get_subplot(self, name):
        return self.plots[name]

    def get_subplots(self):
        return self.plots

    def add_subplot(self, name):
        """ will raise KeyError if: plots exceed 4 """
        self._update_gridspec(len(self.plots) + 1, last=name)
        return self.plots[name]

    def remove_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; there are no plots """
        self.figure.delaxes(self.plots[name])
        del self.plots[name]
        self._update_gridspec(len(self.plots))

    def add_vline(self, plot_name, x_value, y_min, y_max, **kwargs):
        return self.plots[plot_name].axvline(x_value, y_min, y_max, **kwargs)

    def add_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        return self.plots[plot_name].axhline(y_value, x_min, x_max, **kwargs)

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

    def add_errors(self, *plots):
        pass
