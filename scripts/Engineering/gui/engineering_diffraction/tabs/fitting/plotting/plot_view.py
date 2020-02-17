# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets

from mantidqt.plotting.functions import get_plot_fig
from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
from matplotlib.backends.qt_compat import is_pyqt5
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_toolbar import FittingPlotToolbar

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import FigureCanvas
else:
    from matplotlib.backends.backend_qt4agg import FigureCanvas

Ui_plot, _ = load_ui(__file__, "plot_widget.ui")


class FittingPlotView(QtWidgets.QWidget, Ui_plot):
    def __init__(self, parent=None):
        super(FittingPlotView, self).__init__(parent)
        self.setupUi(self)

        self.figure = None
        self.toolbar = None

        self.setup_figure()
        self.setup_toolbar()

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        self.figure.add_subplot(111, projection="mantid")
        self.figure.tight_layout()
        self.toolbar = FittingPlotToolbar(self.figure.canvas, self, False)
        self.vLayout_plot.addWidget(self.toolbar)
        self.vLayout_plot.addWidget(self.figure.canvas)

    def resizeEvent(self, QResizeEvent):
        self.figure.tight_layout()

    def setup_toolbar(self):
        self.toolbar.sig_home_clicked.connect(self.display_all)

    # =================
    # Component Setters
    # =================

    def clear_figure(self):
        self.figure.clf()
        self.figure.add_subplot(111, projection="mantid")
        self.figure.tight_layout()
        self.figure.canvas.draw()

    def update_figure(self):
        self.toolbar.update()
        self.figure.tight_layout()
        self.figure.canvas.draw()

    def display_all(self):
        for ax in self.get_axes():
            if ax.lines:
                ax.relim()
            ax.autoscale()
        self.update_figure()

    # =================
    # Component Getters
    # =================

    def get_axes(self):
        return self.figure.axes

    def get_figure(self):
        return self.figure
