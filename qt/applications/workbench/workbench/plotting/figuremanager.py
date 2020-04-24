# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Provides our custom figure manager to wrap the canvas, window and our custom toolbar"""
import copy
import sys
from functools import wraps
import matplotlib
from matplotlib._pylab_helpers import Gcf
from matplotlib.axes import Axes
from matplotlib.backend_bases import FigureManagerBase
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from matplotlib.collections import LineCollection, QuadMesh
from mpl_toolkits.mplot3d.art3d import Line3DCollection
from mpl_toolkits.mplot3d.axes3d import Axes3D
from qtpy.QtCore import QObject, Qt
from qtpy.QtGui import QColor
from qtpy.QtWidgets import QApplication, QLabel, QFileDialog, QColorDialog, QToolButton, QWidgetAction, QMenu

from mantid.api import AnalysisDataService, AnalysisDataServiceObserver, ITableWorkspace, MatrixWorkspace
from mantid.kernel import logger
from mantid.plots import datafunctions, MantidAxes
from mantid.plots.legend import convert_color_to_hex
from mantidqt.icons import get_icon
from mantidqt.io import open_a_file_dialog
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from mantidqt.widgets.plotconfigdialog import curve_in_ax
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter
from mantidqt.widgets.waterfallplotfillareadialog.presenter import WaterfallPlotFillAreaDialogPresenter
from mantidqt.widgets.waterfallplotoffsetdialog.presenter import WaterfallPlotOffsetDialogPresenter
from workbench.config import get_window_config
from workbench.plotting.figureinteraction import FigureInteraction
from workbench.plotting.figurewindow import FigureWindow
from workbench.plotting.plotscriptgenerator import generate_script
from workbench.plotting.toolbar import WorkbenchNavigationToolbar, ToolbarStateManager


def _catch_exceptions(func):
    """Catch all exceptions in method and print a traceback to stderr"""

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
        self.observeRename(True)

    @_catch_exceptions
    def clearHandle(self):
        """Called when the ADS is deleted all of its workspaces"""
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
        redraw = False
        for ax in all_axes:
            if isinstance(ax, MantidAxes):
                to_redraw = ax.remove_workspace_artists(workspace)
            else:
                to_redraw = False
            if type(ax) is not Axes:  # Solution for filtering out colorbar axes. Works most of the time.
                empty_axes.append(MantidAxes.is_empty(ax))
            redraw = redraw | to_redraw

        if all(empty_axes):
            self.window.emit_close()
        elif redraw:
            self.canvas.draw()

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
            self.canvas.draw()

    @_catch_exceptions
    def renameHandle(self, oldName, newName):
        """
        Called when the ADS has renamed a workspace.
        If this workspace is attached to this figure then the figure name is updated
        :param oldName: The old name of the workspace.
        :param newName: The new name of the workspace
        """
        for ax in self.canvas.figure.axes:
            if isinstance(ax, MantidAxes):
                ws = AnalysisDataService.retrieve(newName)
                if isinstance(ws, MatrixWorkspace):
                    for ws_name, artists in ax.tracked_workspaces.items():
                        if ws_name == oldName:
                            ax.tracked_workspaces[newName] = ax.tracked_workspaces.pop(oldName)
                elif isinstance(ws, ITableWorkspace):
                    ax.wsName = newName


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

        parent, flags = get_window_config()
        self.window = FigureWindow(canvas, parent=parent, window_flags=flags)
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
            self.toolbar.sig_generate_plot_script_clipboard_triggered.connect(
                self.generate_plot_script_clipboard)
            self.toolbar.sig_generate_plot_script_file_triggered.connect(
                self.generate_plot_script_file)
            self.toolbar.sig_home_clicked.connect(self.set_figure_zoom_to_display_all)
            self.toolbar.sig_waterfall_reverse_order_triggered.connect(
                self.waterfall_reverse_line_order)
            self.toolbar.sig_waterfall_offset_amount_triggered.connect(
                self.launch_waterfall_offset_options)
            self.toolbar.sig_waterfall_fill_area_triggered.connect(
                self.launch_waterfall_fill_area_options)
            self.toolbar.sig_waterfall_conversion.connect(self.update_toolbar_waterfall_plot)
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

        self.fit_browser = FitPropertyBrowser(canvas, ToolbarStateManager(self.toolbar))
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
        """Set the canvas size in pixels"""
        self.window.resize(width, height + self._status_and_tool_height)

    def show(self):
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
        if figure_type(self.canvas.figure) not in [FigureType.Line, FigureType.Errorbar] \
                or self.toolbar is not None and len(self.canvas.figure.get_axes()) > 1:
            self._set_fit_enabled(False)

        # For plot-to-script button to show, every axis must be a MantidAxes with lines in it
        # Plot-to-script currently doesn't work with waterfall plots so the button is hidden for that plot type.
        if not all((isinstance(ax, MantidAxes) and curve_in_ax(ax))
                   for ax in self.canvas.figure.get_axes()) or self.canvas.figure.get_axes(
        )[0].is_waterfall():
            self.toolbar.set_generate_plot_script_enabled(False)

        # Only show options specific to waterfall plots if the axes is a MantidAxes and is a waterfall plot.
        if not isinstance(self.canvas.figure.get_axes()[0], MantidAxes) or \
                not self.canvas.figure.get_axes()[0].is_waterfall():
            self.toolbar.set_waterfall_options_enabled(False)

        # For contour and wireframe plots, add a toolbar option to change the colour of the lines.
        if figure_type(self.canvas.figure) == FigureType.Image and \
                any(isinstance(col, LineCollection) for col in self.canvas.figure.get_axes()[0].collections):
            self.set_up_color_selector_toolbar_button()

        if datafunctions.figure_contains_only_3d_plots(self.canvas.figure):
            self.toolbar.adjust_for_3d_plots()

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
        # disconnect window events before calling Gcf.destroy. window.close is not guaranteed to
        # delete the object and do this for us. On macOS it was observed that closing the figure window
        # would produce an extraneous activated event that would add a new figure to the plots list
        # right after deleted the old one.
        self.window.disconnect()
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

    def grid_toggle(self, on):
        """Toggle grid lines on/off"""
        canvas = self.canvas
        axes = canvas.figure.get_axes()
        for ax in axes:
            if not any(isinstance(x, QuadMesh) for x in ax.collections):
                ax.grid(on)
        canvas.draw_idle()

    def fit_toggle(self):
        """Toggle fit browser and tool on/off"""
        if self.fit_browser.isVisible():
            self.fit_browser.hide()
        else:
            self.fit_browser.show()

    def handle_fit_browser_close(self):
        """
        Respond to a signal that user closed self.fit_browser by
        clicking the [x] button.
        """
        self.toolbar.trigger_fit_toggle_action()

    def hold(self):
        """ Mark this figure as held"""
        self.toolbar.hold()

    def get_window_title(self):
        return self.window.windowTitle()

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

    def generate_plot_script_clipboard(self):
        script = generate_script(self.canvas.figure, exclude_headers=True)
        QApplication.clipboard().setText(script)
        logger.notice("Plotting script copied to clipboard.")

    def generate_plot_script_file(self):
        script = generate_script(self.canvas.figure)
        filepath = open_a_file_dialog(parent=self.canvas,
                                      default_suffix=".py",
                                      file_filter="Python Files (*.py)",
                                      accept_mode=QFileDialog.AcceptSave,
                                      file_mode=QFileDialog.AnyFile)
        if filepath:
            try:
                with open(filepath, 'w') as f:
                    f.write(script)
            except IOError as io_error:
                logger.error("Could not write file: {}\n{}" "".format(filepath, io_error))

    def set_figure_zoom_to_display_all(self):
        axes = self.canvas.figure.get_axes()
        if axes:
            for ax in axes:
                # We check for axes type below as a pseudo check for an axes being
                # a colorbar. this is based on the same check in
                # FigureManagerADSObserver.deleteHandle.
                if type(ax) is not Axes:
                    if ax.lines:  # Relim causes issues with colour plots, which have no lines.
                        ax.relim()
                    elif isinstance(ax, Axes3D):
                        if hasattr(ax, 'original_data'):
                            ax.collections[0]._vec = copy.deepcopy(ax.original_data)
                        else:
                            ax.view_init()

                    ax.autoscale()

            self.canvas.draw()

    def waterfall_reverse_line_order(self):
        ax = self.canvas.figure.get_axes()[0]
        x, y = ax.waterfall_x_offset, ax.waterfall_y_offset
        fills = datafunctions.get_waterfall_fills(ax)
        ax.update_waterfall(0, 0)

        errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(ax)

        ax.lines.reverse()
        ax.lines += errorbar_cap_lines
        ax.collections += fills
        ax.collections.reverse()
        ax.update_waterfall(x, y)

        if ax.get_legend():
            ax.make_legend()

    def launch_waterfall_offset_options(self):
        WaterfallPlotOffsetDialogPresenter(self.canvas.figure, parent=self.window)

    def launch_waterfall_fill_area_options(self):
        WaterfallPlotFillAreaDialogPresenter(self.canvas.figure, parent=self.window)

    def update_toolbar_waterfall_plot(self, is_waterfall):
        self.toolbar.set_waterfall_options_enabled(is_waterfall)
        self.toolbar.set_generate_plot_script_enabled(not is_waterfall)

    def set_up_color_selector_toolbar_button(self):
        a = self.toolbar.addAction(get_icon('mdi.palette'), "Line Colour", lambda: None)

        ax_collections = self.canvas.figure.get_axes()[0].collections
        if any(isinstance(col, Line3DCollection) for col in ax_collections):
            a.setToolTip("Set the colour of the wireframe.")
        else:
            a.setToolTip("Set the colour of the contour lines.")

        line_collection = next(col for col in ax_collections if isinstance(col, LineCollection))
        initial_colour = convert_color_to_hex(line_collection.get_color()[0])
        initial_q_colour = QColor(initial_colour)

        colour_dialog = QColorDialog()
        colour_dialog.setOption(QColorDialog.NoButtons)
        colour_dialog.setCurrentColor(initial_q_colour)
        colour_dialog.currentColorChanged.connect(self.change_line_collection_colour)

        button = [child for child in self.toolbar.children() if isinstance(child, QToolButton)][-1]

        menu = QMenu("Menu", parent=button)
        colour_selector_action = QWidgetAction(menu)
        colour_selector_action.setDefaultWidget(colour_dialog)
        menu.addAction(colour_selector_action)

        button.setMenu(menu)
        button.setPopupMode(QToolButton.InstantPopup)

    def change_line_collection_colour(self, colour):
        from matplotlib.collections import LineCollection
        for col in self.canvas.figure.get_axes()[0].collections:
            if isinstance(col, LineCollection):
                col.set_color(colour.name())

        self.canvas.draw()


# -----------------------------------------------------------------------------
# Figure control
# -----------------------------------------------------------------------------


def new_figure_manager(num, *args, **kwargs):
    """Create a new figure manager instance"""
    from matplotlib.figure import Figure  # noqa
    figure_class = kwargs.pop('FigureClass', Figure)
    this_fig = figure_class(*args, **kwargs)
    return new_figure_manager_given_figure(num, this_fig)


def new_figure_manager_given_figure(num, figure):
    """Create a new figure manager instance for the given figure."""
    canvas = FigureCanvasQTAgg(figure)
    manager = FigureManagerWorkbench(canvas, num)
    return manager
