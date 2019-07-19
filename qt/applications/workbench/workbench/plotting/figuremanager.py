# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Provides our custom figure manager to wrap the canvas, window and our custom toolbar"""
from __future__ import (absolute_import, unicode_literals)

# 3rdparty imports
import matplotlib
import numpy as np
import sys
from functools import wraps
from matplotlib._pylab_helpers import Gcf
from matplotlib.axes import Axes
from matplotlib.backend_bases import FigureManagerBase
from matplotlib.backends.backend_qt5agg import (FigureCanvasQTAgg)  # noqa
from qtpy.QtCore import QObject, Qt
from qtpy.QtWidgets import QApplication, QLabel

# local imports
from mantid.api import AnalysisDataServiceObserver
from mantid.plots import MantidAxes
from mantid.py3compat import text_type
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter
from .figureinteraction import FigureInteraction
from .figurewindow import FigureWindow
from .toolbar import WorkbenchNavigationToolbar, ToolbarStateManager


def _catch_exceptions(func):
    """
    Catch all exceptions in method and print a traceback to stderr
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception:
            sys.stderr.write("Error occurred in handler:\n")
            import traceback
            traceback.print_exc()

    return wrapper


class FigureManagerADSObserver(AnalysisDataServiceObserver):

    def __init__(self, manager):
        super(FigureManagerADSObserver, self).__init__()
        self.window = manager.window
        self.canvas = manager.canvas

        self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(True)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS is deleted all of its workspaces
        """
        self.window.emit_close()

    @_catch_exceptions
    def deleteHandle(self, _, workspace):
        """
        Called when the ADS has deleted a workspace. Checks the
        attached axes for any hold a plot from this workspace. If removing
        this leaves empty axes then the parent window is triggered for
        closer
        :param _: The name of the workspace. Unused
        :param workspace: A pointer to the workspace
        """
        # Find the axes with this workspace reference
        all_axes = self.canvas.figure.axes
        if not all_axes:
            return

        # Here we wish to delete any curves linked to the workspace being
        # deleted and if a figure is now empty, close it. We must avoid closing
        # any figures that were created via the script window that are not
        # managed via a workspace.
        # See https://github.com/mantidproject/mantid/issues/25135.
        empty_axes = []
        for ax in all_axes:
            if isinstance(ax, MantidAxes):
                ax.remove_workspace_artists(workspace)
            # We check for axes type below as a pseudo check for an axes being
            # a colorbar. Creating a colorfill plot creates 2 axes: one linked
            # to a workspace, the other a colorbar. Deleting the workspace
            # deletes the colorfill, but the plot remains open due to the
            # non-empty colorbar. This solution seems to work for the majority
            # of cases but could lead to unmanaged figures only containing an
            # Axes object being closed.
            if type(ax) is not Axes:
                empty_axes.append(MantidAxes.is_empty(ax))

        if all(empty_axes):
            self.window.emit_close()
        else:
            self.canvas.draw_idle()

    @_catch_exceptions
    def replaceHandle(self, _, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached to this figure then its data is updated
        :param _: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        redraw = False
        for ax in self.canvas.figure.axes:
            if isinstance(ax, MantidAxes):
                redraw_this = ax.replace_workspace_artists(workspace)
            else:
                continue
            redraw = redraw | redraw_this
        if redraw:
            self.canvas.draw_idle()


class FigureManagerWorkbench(FigureManagerBase, QObject):
    """
    Attributes
    ----------
    canvas : `FigureCanvas`
        The FigureCanvas instance
    num : int or str
        The Figure number
    toolbar : qt.QToolBar
        The qt.QToolBar
    window : qt.QMainWindow
        The qt.QMainWindow

    """

    def __init__(self, canvas, num):
        QObject.__init__(self)
        FigureManagerBase.__init__(self, canvas, num)
        # Patch show/destroy to be thread aware
        self._destroy_orig = self.destroy
        self.destroy = QAppThreadCall(self._destroy_orig)
        self._show_orig = self.show
        self.show = QAppThreadCall(self._show_orig)
        self._window_activated_orig = self._window_activated
        self._window_activated = QAppThreadCall(self._window_activated_orig)
        self.set_window_title_orig = self.set_window_title
        self.set_window_title = QAppThreadCall(self.set_window_title_orig)
        self.fig_visibility_changed_orig = self.fig_visibility_changed
        self.fig_visibility_changed = QAppThreadCall(self.fig_visibility_changed_orig)

        self.window = FigureWindow(canvas)
        self.window.activated.connect(self._window_activated)
        self.window.closing.connect(canvas.close_event)
        self.window.closing.connect(self.destroy)
        self.window.visibility_changed.connect(self.fig_visibility_changed)

        self.window.setWindowTitle("Figure %d" % num)
        canvas.figure.set_label("Figure %d" % num)

        # Give the keyboard focus to the figure instead of the
        # manager; StrongFocus accepts both tab and click to focus and
        # will enable the canvas to process event w/o clicking.
        # ClickFocus only takes the focus is the window has been
        # clicked
        # on. http://qt-project.org/doc/qt-4.8/qt.html#FocusPolicy-enum or
        # http://doc.qt.digia.com/qt/qt.html#FocusPolicy-enum
        canvas.setFocusPolicy(Qt.StrongFocus)
        canvas.setFocus()

        self.window._destroying = False

        # add text label to status bar
        self.statusbar_label = QLabel()
        self.window.statusBar().addWidget(self.statusbar_label)

        self.plot_options_dialog = None
        self.toolbar = self._get_toolbar(canvas, self.window)
        if self.toolbar is not None:
            self.window.addToolBar(self.toolbar)
            self.toolbar.message.connect(self.statusbar_label.setText)
            self.toolbar.sig_grid_toggle_triggered.connect(self.grid_toggle)
            self.toolbar.sig_toggle_fit_triggered.connect(self.fit_toggle)
            self.toolbar.sig_plot_options_triggered.connect(self.launch_plot_options)
            self.toolbar.setFloatable(False)
            tbs_height = self.toolbar.sizeHint().height()
        else:
            tbs_height = 0

        # resize the main window so it will display the canvas with the
        # requested size:
        cs = canvas.sizeHint()
        sbs = self.window.statusBar().sizeHint()
        self._status_and_tool_height = tbs_height + sbs.height()
        height = cs.height() + self._status_and_tool_height
        self.window.resize(cs.width(), height)

        self.fit_browser = FitPropertyBrowser(canvas,
                                              ToolbarStateManager(self.toolbar))
        self.fit_browser.closing.connect(self.handle_fit_browser_close)
        self.window.setCentralWidget(canvas)
        self.window.addDockWidget(Qt.LeftDockWidgetArea, self.fit_browser)
        self.fit_browser.hide()

        if matplotlib.is_interactive():
            self.window.show()
            canvas.draw_idle()

        def notify_axes_change(fig):
            # This will be called whenever the current axes is changed
            if self.toolbar is not None:
                self.toolbar.update()

        canvas.figure.add_axobserver(notify_axes_change)

        # Register canvas observers
        self._fig_interaction = FigureInteraction(self)
        self._ads_observer = FigureManagerADSObserver(self)

        self.window.raise_()

    def full_screen_toggle(self):
        if self.window.isFullScreen():
            self.window.showNormal()
        else:
            self.window.showFullScreen()

    def _window_activated(self):
        Gcf.set_active(self)

    def _get_toolbar(self, canvas, parent):
        return WorkbenchNavigationToolbar(canvas, parent, False)

    def resize(self, width, height):
        'set the canvas size in pixels'
        self.window.resize(width, height + self._status_and_tool_height)

    def show(self):  # noqa
        self.window.show()
        self.window.activateWindow()
        self.window.raise_()
        if self.window.windowState() & Qt.WindowMinimized:
            # windowState() stores a combination of window state enums
            # and multiple window states can be valid. On Windows
            # a window can be both minimized and maximized at the
            # same time, so we make a check here. For more info see:
            # http://doc.qt.io/qt-5/qt.html#WindowState-enum
            if self.window.windowState() & Qt.WindowMaximized:
                self.window.setWindowState(Qt.WindowMaximized)
            else:
                self.window.setWindowState(Qt.WindowNoState)

        # Hack to ensure the canvas is up to date
        self.canvas.draw_idle()
        if figure_type(self.canvas.figure) not in [FigureType.Line, FigureType.Errorbar]:
            self._set_fit_enabled(False)

    def destroy(self, *args):
        # check for qApp first, as PySide deletes it in its atexit handler
        if QApplication.instance() is None:
            return

        if self.window._destroying:
            return
        self.window._destroying = True

        if self.toolbar:
            self.toolbar.destroy()
        self._ads_observer.observeAll(False)
        del self._ads_observer
        self._fig_interaction.disconnect()
        self.window.close()

        try:
            Gcf.destroy(self.num)
        except AttributeError:
            pass
            # It seems that when the python session is killed,
            # Gcf can get destroyed before the Gcf.destroy
            # line is run, leading to a useless AttributeError.

    def launch_plot_options(self):
        self.plot_options_dialog = PlotConfigDialogPresenter(self.canvas.figure, parent=self.window)

    def grid_toggle(self):
        """
        Toggle grid lines on/off
        """
        canvas = self.canvas
        axes = canvas.figure.get_axes()
        for ax in axes:
            ax.grid()
        canvas.draw_idle()

    def fit_toggle(self):
        """
        Toggle fit browser and tool on/off
        """
        if self.fit_browser.isVisible():
            self.fit_browser.hide()
        else:
            self.fit_browser.show()

    def handle_fit_browser_close(self):
        """
        Respond to a signal that user closed self.fit_browser by clicking the [x] button.
        """
        self.toolbar.trigger_fit_toggle_action()

    def hold(self):
        """
        Mark this figure as held
        """
        self.toolbar.hold()

    def get_window_title(self):
        return text_type(self.window.windowTitle())

    def set_window_title(self, title):
        self.window.setWindowTitle(title)
        # We need to add a call to the figure manager here to call
        # notify methods when a figure is renamed, to update our
        # plot list.
        Gcf.figure_title_changed(self.num)

        # For the workbench we also keep the label in sync, this is
        # to allow getting a handle as plt.figure('Figure Name')
        self.canvas.figure.set_label(title)

    def fig_visibility_changed(self):
        """
        Make a notification in the global figure manager that
        plot visibility was changed. This method is added to this
        class so that it can be wrapped in a QAppThreadCall.
        """
        Gcf.figure_visibility_changed(self.num)

    def _set_fit_enabled(self, on):
        action = self.toolbar._actions['toggle_fit']
        action.setEnabled(on)
        action.setVisible(on)


# -----------------------------------------------------------------------------
# Figure control
# -----------------------------------------------------------------------------

def new_figure_manager(num, *args, **kwargs):
    """
    Create a new figure manager instance
    """
    from matplotlib.figure import Figure  # noqa
    figure_class = kwargs.pop('FigureClass', Figure)
    this_fig = figure_class(*args, **kwargs)
    return new_figure_manager_given_figure(num, this_fig)


def new_figure_manager_given_figure(num, figure):
    """
    Create a new figure manager instance for the given figure.
    """
    canvas = FigureCanvasQTAgg(figure)
    manager = FigureManagerWorkbench(canvas, num)
    return manager


if __name__ == '__main__':
    # testing code

    qapp = QApplication([' '])
    qapp.setAttribute(Qt.AA_UseHighDpiPixmaps)
    if hasattr(Qt, 'AA_EnableHighDpiScaling'):
        qapp.setAttribute(Qt.AA_EnableHighDpiScaling, True)

    x = np.linspace(0, 10 * np.pi, 1000)
    cx, sx = np.cos(x), np.sin(x)
    fig_mgr_1 = new_figure_manager(1)
    fig1 = fig_mgr_1.canvas.figure
    ax = fig1.add_subplot(111)
    ax.set_title("Test title")
    ax.set_xlabel(r"$\mu s$")
    ax.set_ylabel("Counts")

    ax.plot(x, cx)
    fig1.show()
    qapp.exec_()
