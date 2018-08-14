from Muon.GUI.ElementalAnalysis.Plotting.plotting_utils import AxisChanger

#from mantid import plots

from matplotlib.figure import Figure
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
# pyplot should not be imported:
# https://stackoverflow.com/posts/comments/26295260

from PyQt4 import QtGui


class PlotView(QtGui.QWidget):
    def __init__(self):
        super(PlotView, self).__init__()
        plots = 0
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)

        self.plot_selector = QtGui.QComboBox()
        test_button = QtGui.QPushButton("test")
        test_button.clicked.connect(self.plot)

        button_layout = QtGui.QHBoxLayout()
        self.x_axis_changer = AxisChanger("X")
        self.y_axis_changer = AxisChanger("Y")
        button_layout.addWidget(test_button)
        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer)
        button_layout.addWidget(self.y_axis_changer)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.canvas, 0, 0)
        grid.addLayout(button_layout, 1, 0)
        self.setLayout(grid)

    def plot(self):
        data = [i**0.3 for i in range(100)]
        a = self.figure.add_subplot(221)
        a.plot(data)
        b = self.figure.add_subplot(222)
        b.plot(data[::-1])
        self.canvas.draw()
