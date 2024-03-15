# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QDockWidget, QMainWindow, QMenu, QSizePolicy
from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
from mantidqt.MPLwidgets import FigureCanvas
from .EngDiff_fitpropertybrowser import EngDiffFitPropertyBrowser
from workbench.plotting.toolbar import ToolbarStateManager
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_toolbar import FittingPlotToolbar
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObserver

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
        self.initial_chart_width = None
        self.initial_chart_height = None
        self.has_first_undock_occurred = 0

        self.setup_figure()

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        self.figure.canvas.mpl_connect("button_press_event", self.mouse_click)
        self.figure.add_subplot(111, projection="mantid")
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
        self.plot_dock.topLevelChanged.connect(self.make_undocked_plot_larger)
        self.initial_chart_width, self.initial_chart_height = self.plot_dock.width(), self.plot_dock.height()
        self.plot_dock.setSizePolicy(QSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding))
        self.dock_window.addDockWidget(Qt.BottomDockWidgetArea, self.plot_dock)
        self.vLayout_plot.addWidget(self.dock_window)

        self.fit_browser = EngDiffFitPropertyBrowser(self.figure.canvas, ToolbarStateManager(self.toolbar))
        # remove SequentialFit from fit menu (implemented a different way)
        qmenu = self.fit_browser.getFitMenu()
        qmenu.removeAction([qact for qact in qmenu.actions() if qact.text() == "Sequential Fit"][0])
        # hide unnecessary properties of browser
        hide_props = [
            "Minimizer",
            "Cost function",
            "Max Iterations",
            "Output",
            "Ignore invalid data",
            "Peak Radius",
            "Plot Composite Members",
            "Convolve Composite Members",
            "Show Parameter Errors",
            "Evaluate Function As",
        ]
        self.fit_browser.removePropertiesFromSettingsBrowser(hide_props)
        self.fit_browser.toggleWsListVisible()
        self.fit_browser.closing.connect(self.toolbar.handle_fit_browser_close)

        fit_enabled_observer = GenericObserverWithArgPassing(self.set_sequential_serial_fit_enabled)
        self.fit_browser.fit_enabled_notifier.add_subscriber(fit_enabled_observer)

        self.vLayout_fitprop.addWidget(self.fit_browser)
        self.hide_fit_browser()
        self.hide_fit_progress_bar()
        self.show_cancel_button(False)
        self.set_find_peaks_convolve_button_status(False)

    def mouse_click(self, event):
        if event.button == event.canvas.buttond.get(Qt.RightButton):
            menu = QMenu()
            self.fit_browser.add_to_menu(menu)
            menu.exec_(QCursor.pos())

    def resizeEvent(self, QResizeEvent):
        self.update_axes_position()

    def make_undocked_plot_larger(self):
        # only make undocked plot larger the first time it is undocked as the undocked size gets remembered
        if self.plot_dock.isFloating() and self.has_first_undock_occurred == 0:
            factor = 1.0
            aspect_ratio = self.initial_chart_width / self.initial_chart_height
            new_height = self.initial_chart_height * factor
            docked_height = self.dock_window.height()
            if docked_height > new_height:
                new_height = docked_height
            new_width = new_height * aspect_ratio
            self.plot_dock.resize(int(new_width), int(new_height))
            self.has_first_undock_occurred = 1

        self.update_axes_position()

    def ensure_fit_dock_closed(self):
        if self.plot_dock.isFloating():
            self.plot_dock.close()

    # ===============
    # Slot Connectors
    # ===============

    def set_cancel_clicked(self, slot):
        self.cancel_button.clicked.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_slot_for_fit_toggled(self, presenter_func):
        self.toolbar.sig_toggle_fit_triggered.connect(presenter_func)

    def set_slot_for_display_all(self):
        self.toolbar.sig_home_clicked.connect(self.display_all)

    def set_slot_for_serial_fit(self, presenter_func):
        self.toolbar.sig_serial_fit_clicked.connect(presenter_func)

    def set_slot_for_seq_fit(self, presenter_func):
        self.toolbar.sig_seq_fit_clicked.connect(presenter_func)

    def set_slot_for_legend_toggled(self):
        self.toolbar.sig_toggle_legend.connect(self.toggle_legend)

    def set_slot_for_find_peaks_convolve(self, presenter_func):
        self.toolbar.sig_find_peaks_convolve.connect(presenter_func)

    def set_subscriber_for_function_changed(self, presenter_func):
        func_changed_observer = GenericObserver(presenter_func)
        self.fit_browser.function_changed_notifier.add_subscriber(func_changed_observer)

    def toggle_legend(self):
        self.update_legend(self.get_axes()[0])
        self.figure.canvas.draw()

    def show_cancel_button(self, show: bool):
        self.cancel_button.setVisible(show)
        self.cancel_button.setEnabled(show)

    def hide_fit_browser(self):
        self.fit_browser.hide()

    def show_fit_browser(self):
        self.fit_browser.show()

    def hide_fit_progress_bar(self):
        self.fit_progress_bar.hide()

    def set_find_peaks_convolve_button_status(self, status):
        if self.fit_browser.isVisible() is False:
            self.toolbar.set_action_enabled("FindPeaksConvolve", False)
        else:
            self.toolbar.set_action_enabled("FindPeaksConvolve", status)

    def show_fit_progress_bar(self):
        self.fit_progress_bar.show()

    def set_progress_bar(self, status, minimum, maximum, value, style_sheet):
        self.fit_progress_bar.setToolTip(str(status))
        self.fit_progress_bar.setMinimum(minimum)
        self.fit_progress_bar.setMaximum(maximum)
        self.fit_progress_bar.setValue(value)
        self.fit_progress_bar.setStyleSheet(style_sheet)

    def clear_figure(self):
        self.figure.clf()
        self.figure.add_subplot(111, projection="mantid")
        self.figure.canvas.draw()

    def update_figure(self):
        self.toolbar.update()
        self.update_legend(self.get_axes()[0])
        self.update_axes_position()
        self.figure.canvas.draw()
        self.update_fitbrowser()

    def update_axes_position(self):
        """
        Set axes position so that labels are always visible - it deliberately ignores height of ylabel (and less
        importantly the length of xlabel). This is because the plot window typically has a very small height when docked
        in the UI.
        """
        ax = self.get_axes()[0]
        y0_lab = (
            ax.xaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(self.figure.transFigure.inverted()).y0
        )  # vertical coord of bottom left corner of xlabel in fig ref. frame
        x0_lab = (
            ax.yaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(self.figure.transFigure.inverted()).x0
        )  # horizontal coord of bottom left corner ylabel in fig ref. frame
        pos = ax.get_position()
        x0_ax = pos.x0 + 0.05 - x0_lab  # move so that ylabel left bottom corner at horizontal coord 0.05
        y0_ax = pos.y0 + 0.05 - y0_lab  # move so that xlabel left bottom corner at vertical coord 0.05
        ax.set_position([x0_ax, y0_ax, 0.95 - x0_ax, 0.95 - y0_ax])

    def update_fitbrowser(self):
        is_visible = self.fit_browser.isVisible()
        self.toolbar.set_fit_checkstate(False)
        self.fit_browser.hide()
        self.fit_progress_bar.hide()
        if self.fit_browser.getWorkspaceNames() and is_visible:
            self.toolbar.set_fit_checkstate(True)
            self.fit_browser.show()
            self.fit_progress_bar.show()

    def remove_ws_from_fitbrowser(self, ws):
        # only one spectrum per workspace
        try:
            self.fit_browser.removeWorkspaceAndSpectra(ws.name())
        except:
            pass  # name may not be available if ws has just been deleted

    def update_legend(self, ax):
        if ax.get_lines():
            ax.make_legend()
            ax.get_legend().set_title("")
            ax.get_legend().set_visible(self.toolbar.get_show_legend_value())
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
        if status:
            self.fit_browser.fitResultsChanged.emit(status)
            self.fit_browser.changeWindowTitle.emit(status)
            # update browser with output function and save setup if successful
            if "success" in status.lower():
                self.fit_browser.loadFunction(func_str)
                self.fit_browser.save_current_setup(setup_name)

    def set_sequential_serial_fit_enabled(self, fit_enabled):
        self.toolbar.set_action_enabled("Serial Fit", fit_enabled)
        self.toolbar.set_action_enabled("Sequential Fit", fit_enabled)

    # =================
    # Component Getters
    # =================

    def get_axes(self):
        return self.figure.axes

    def get_figure(self):
        return self.figure

    def is_fit_browser_visible(self):
        return self.fit_browser.isVisible()
