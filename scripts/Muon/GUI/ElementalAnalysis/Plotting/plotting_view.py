from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils
from Muon.GUI.Common.message_box import warning

#from mantid import plots

from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from PyQt4 import QtGui


class PlotView(QtGui.QWidget):
    def __init__(self):
        super(PlotView, self).__init__()
        self.plots = {}
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
        add_button = QtGui.QPushButton("add")
        add_button.clicked.connect(self.plot)

        del_button = QtGui.QPushButton("del")
        del_button.clicked.connect(self.del_plot)

        button_layout = QtGui.QHBoxLayout()
        self.x_axis_changer = putils.AxisChanger("X")
        self.y_axis_changer = putils.AxisChanger("Y")
        button_layout.addWidget(add_button)
        button_layout.addWidget(del_button)
        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer)
        button_layout.addWidget(self.y_axis_changer)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.canvas, 0, 0)
        grid.addLayout(button_layout, 1, 0)
        self.setLayout(grid)

    def _set_positions(self, grid, positions):
        for plot, pos in zip(self.plots.values(), positions):
            plot.set_position(
                grid[pos[0], pos[1]].get_position(self.figure))  # magic

    def add_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; plots exceed 4 """
        new_plots = len(self.plots) + 1
        g = self.gridspecs[new_plots]
        positions = putils.get_layout(new_plots)
        self._set_positions(g, positions)
        self.plots[name] = self.figure.add_subplot(
            g[positions[-1][0], positions[-1][1]])
        self.canvas.draw()
        return self.plots[name]

    def remove_subplot(self, name):
        """ will raise KeyError if: 'name' isn't a plot; there are no plots """
        try:
            self.figure.delaxes(self.plots[name])
            del self.plots[name]
            new_plots = len(self.plots)
            if new_plots:
                g = self.gridspecs[new_plots]
                positions = putils.get_layout(new_plots)
                self._set_positions(g, positions)
            self.canvas.draw()
        except KeyError as e:
            warning(e)

    def plot(self):
        self.add_subplot(str(len(self.plots) + 1))

    def del_plot(self):
        try:
            # self.remove_subplot(random.choice(self.plots.keys())) breaks?
            self.remove_subplot(str(len(self.plots)))
        except BaseException:
            return
