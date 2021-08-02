# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QDockWidget, QMainWindow, QMenu
from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
from mantidqt.MPLwidgets import FigureCanvas
from .EngDiff_fitpropertybrowser import EngDiffFitPropertyBrowser
from workbench.plotting.toolbar import ToolbarStateManager
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_toolbar import FittingPlotToolbar

Ui_plot, _ = load_ui(__file__, "plot_widget.ui")


class FittingPlotView(QtWidgets.QWidget, Ui_plot):
    def __init__(self, parent=None):
        super(FittingPlotView, self).__init__(parent)
        self.setupUi(self)

        self.figure = None
        self.toolbar = None
        self.fitprop_toolbar = None
        self.fit_browser = None
        self.plot_dock = None
        self.dock_window = None

        self.setup_figure()
        self.setup_toolbar()

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        self.figure.canvas.mpl_connect('button_press_event', self.mouse_click)
        self.figure.add_subplot(111, projection="mantid")
        self.figure.tight_layout()
        self.toolbar = FittingPlotToolbar(self.figure.canvas, self, False)
        self.toolbar.setMovable(False)

        self.dock_window = QMainWindow(self.group_plot)
        self.dock_window.setWindowFlags(Qt.Widget)
        self.dock_window.setDockOptions(QMainWindow.AnimatedDocks)
        self.dock_window.setCentralWidget(self.toolbar)
        self.plot_dock = QDockWidget()
        self.plot_dock.setWidget(self.figure.canvas)
        self.plot_dock.setFeatures(QDockWidget.DockWidgetFloatable | QDockWidget.DockWidgetMovable)
        self.plot_dock.setAllowedAreas(Qt.BottomDockWidgetArea)
        self.plot_dock.setWindowTitle("Fit Plot")
        self.dock_window.addDockWidget(Qt.BottomDockWidgetArea, self.plot_dock)
        self.vLayout_plot.addWidget(self.dock_window)

        self.fit_browser = EngDiffFitPropertyBrowser(self.figure.canvas,
                                                     ToolbarStateManager(self.toolbar))
        # remove SequentialFit from fit menu (implemented a different way)
        qmenu = self.fit_browser.getFitMenu()
        qmenu.removeAction([qact for qact in qmenu.actions() if qact.text() == "Sequential Fit"][0])
        # hide unnecessary properties of browser
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

    def ensure_fit_dock_closed(self):
        if self.plot_dock.isFloating():
            self.plot_dock.close()

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

    def read_fitprop_from_browser(self):
        return self.fit_browser.read_current_fitprop()

    def update_browser(self, status, func_str, setup_name):
        self.fit_browser.fitResultsChanged.emit(status)
        self.fit_browser.changeWindowTitle.emit(status)
        # update browser with output function and save setup if successful
        if "success" in status.lower():
            self.fit_browser.loadFunction(func_str)
            self.fit_browser.save_current_setup(setup_name)

    # =================
    # Component Getters
    # =================

    def get_axes(self):
        return self.figure.axes

    def get_figure(self):
        return self.figure
