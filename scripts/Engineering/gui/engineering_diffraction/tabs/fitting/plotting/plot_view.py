# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QMenu
from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
from matplotlib.backends.qt_compat import is_pyqt5
from .EngDiff_fitpropertybrowser import EngDiffFitPropertyBrowser
from workbench.plotting.toolbar import ToolbarStateManager
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
        self.fitprop_toolbar = None
        self.fit_browser = None

        self.setup_figure()
        self.setup_toolbar()

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        self.figure.canvas.mpl_connect('button_press_event', self.mouse_click)
        self.figure.add_subplot(111, projection="mantid")
        self.figure.tight_layout()
        self.toolbar = FittingPlotToolbar(self.figure.canvas, self, False)
        self.vLayout_plot.addWidget(self.toolbar)
        self.vLayout_plot.addWidget(self.figure.canvas)
        self.fit_browser = EngDiffFitPropertyBrowser(self.figure.canvas,
                                                     ToolbarStateManager(self.toolbar))
        hide_props = ['StartX', 'EndX', 'Minimizer', 'Cost function', 'Max Iterations', 'Output',
                      'Ignore invalid data', 'Peak Radius', 'Plot Composite Members',
                      'Convolve Composite Members', 'Show Parameter Errors', 'Evaluate Function As']
        self.fit_browser.removePropertiesFromSettingsBrowser(hide_props)
        self.fit_browser.toggleWsListVisible()
        self.fit_browser.closing.connect(self.toolbar.handle_fit_browser_close)
        self.vLayout_fitprop.addWidget(self.fit_browser)
        self.fit_browser.hide()

    def mouse_click(self, event):
        if event.button == event.canvas.buttond.get(Qt.RightButton):
            menu = QMenu()
            self.fit_browser.add_to_menu(menu)
            menu.exec_(QCursor.pos())

    def resizeEvent(self, QResizeEvent):
        self.figure.tight_layout()

    def setup_toolbar(self):
        self.toolbar.sig_home_clicked.connect(self.display_all)
        self.toolbar.sig_toggle_fit_triggered.connect(self.fit_toggle)

    # =================
    # Component Setters
    # =================

    def fit_toggle(self):
        """Toggle fit browser and tool on/off"""
        if self.fit_browser.isVisible():
            self.fit_browser.hide()
        else:
            self.fit_browser.show()

    def clear_figure(self):
        self.figure.clf()
        self.figure.add_subplot(111, projection="mantid")
        self.figure.tight_layout()
        self.figure.canvas.draw()

    def update_figure(self):
        self.toolbar.update()
        self.figure.tight_layout()
        self.update_legend(self.get_axes()[0])
        self.figure.canvas.draw()
        self.update_fitbrowser()

    def update_fitbrowser(self):
        is_visible = self.fit_browser.isVisible()
        self.toolbar.set_fit_checkstate(False)
        self.fit_browser.hide()
        if self.fit_browser.getWorkspaceNames() and is_visible:
            self.toolbar.set_fit_checkstate(True)
            self.fit_browser.show()

    def remove_ws_from_fitbrowser(self, ws):
        # only one spectra per workspace
        self.fit_browser.removeWorkspaceAndSpectra(ws.name())

    def update_legend(self, ax):
        if ax.get_lines():
            ax.make_legend()
            ax.get_legend().set_title("")
        else:
            if ax.get_legend():
                ax.get_legend().remove()

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
