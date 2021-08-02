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
from distutils.version import LooseVersion
import io
import sys
import re
from functools import wraps

import matplotlib
from matplotlib._pylab_helpers import Gcf
from matplotlib.axes import Axes
from matplotlib.backend_bases import FigureManagerBase
from matplotlib.collections import LineCollection
from mpl_toolkits.mplot3d.axes3d import Axes3D
from qtpy.QtCore import QObject, Qt
from qtpy.QtGui import QImage
from qtpy.QtWidgets import QApplication, QLabel, QFileDialog
from qtpy import QT_VERSION

from mantid.api import AnalysisDataService, AnalysisDataServiceObserver, ITableWorkspace, MatrixWorkspace
from mantid.kernel import logger
from mantid.plots import datafunctions, MantidAxes, axesfunctions
from mantidqt.io import open_a_file_dialog
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall, force_method_calls_to_qapp_thread
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter
from mantidqt.widgets.superplot import Superplot
from mantidqt.widgets.waterfallplotfillareadialog.presenter import WaterfallPlotFillAreaDialogPresenter
from mantidqt.widgets.waterfallplotoffsetdialog.presenter import WaterfallPlotOffsetDialogPresenter
from workbench.config import get_window_config
from workbench.plotting.mantidfigurecanvas import (  # noqa: F401
    MantidFigureCanvas, draw_if_interactive as draw_if_interactive_impl, show as show_impl)
from workbench.plotting.figureinteraction import FigureInteraction
from workbench.plotting.figurewindow import FigureWindow
from workbench.plotting.plotscriptgenerator import generate_script
from workbench.plotting.toolbar import WorkbenchNavigationToolbar, ToolbarStateManager
from workbench.plotting.plothelppages import PlotHelpPages


def _replace_workspace_name_in_string(old_name, new_name, string):
    return re.sub(rf'\b{old_name}\b', new_name, string)


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
            # Solution for filtering out colorbar axes. Works most of the time.
            if type(ax) is not Axes:
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
        If this workspace is attached to this figure then the figure name is updated, as are the artists names and
        axis creation arguments
        :param oldName: The old name of the workspace.
        :param newName: The new name of the workspace
        """
        for ax in self.canvas.figure.axes:
            if isinstance(ax, MantidAxes):
                ws = AnalysisDataService.retrieve(newName)
                if isinstance(ws, MatrixWorkspace):
                    ax.rename_workspace(newName, oldName)
                elif isinstance(ws, ITableWorkspace):
                    ax.wsName = newName
                ax.make_legend()
            ax.set_title(_replace_workspace_name_in_string(oldName, newName, ax.get_title()))
        self.canvas.set_window_title(
            _replace_workspace_name_in_string(oldName, newName, self.canvas.get_window_title()))
        self.canvas.draw()


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
        assert QAppThreadCall.is_qapp_thread(
        ), "FigureManagerWorkbench cannot be created outside of the QApplication thread"
        QObject.__init__(self)

        parent, flags = get_window_config()
        self.window = FigureWindow(canvas, parent=parent, window_flags=flags)
        self.window.activated.connect(self._window_activated)
        self.window.closing.connect(canvas.close_event)
        self.window.closing.connect(self.destroy)
        self.window.visibility_changed.connect(self.fig_visibility_changed)

        self.window.setWindowTitle("Figure %d" % num)
        canvas.figure.set_label("Figure %d" % num)

        FigureManagerBase.__init__(self, canvas, num)
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
            self.toolbar.sig_toggle_superplot_triggered.connect(self.superplot_toggle)
            self.toolbar.sig_copy_to_clipboard_triggered.connect(self.copy_to_clipboard)
            self.toolbar.sig_plot_options_triggered.connect(self.launch_plot_options)
            self.toolbar.sig_plot_help_triggered.connect(self.launch_plot_help)
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
            self.toolbar.sig_change_line_collection_colour_triggered.connect(
                self.change_line_collection_colour)
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

        self.superplot = None

        # Need this line to stop the bug where the dock window snaps back to its original size after resizing.
        # 0 argument is arbitrary and has no effect on fit widget size
        # This is a qt bug reported at (https://bugreports.qt.io/browse/QTBUG-65592)
        if QT_VERSION >= LooseVersion("5.6"):
            self.window.resizeDocks([self.fit_browser], [1], Qt.Horizontal)
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

        if self.toolbar:
            self.toolbar.set_buttons_visibility(self.canvas.figure)

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
        self._ads_observer = None
        # disconnect window events before calling Gcf.destroy. window.close is not guaranteed to
        # delete the object and do this for us. On macOS it was observed that closing the figure window
        # would produce an extraneous activated event that would add a new figure to the plots list
        # right after deleted the old one.
        self.window.disconnect()
        self._fig_interaction.disconnect()
        self.window.close()
        if self.superplot:
            self.superplot.close()

        try:
            Gcf.destroy(self.num)
        except AttributeError:
            pass
            # It seems that when the python session is killed,
            # Gcf can get destroyed before the Gcf.destroy
            # line is run, leading to a useless AttributeError.

    def launch_plot_options(self):
        self.plot_options_dialog = PlotConfigDialogPresenter(self.canvas.figure, parent=self.window)

    def launch_plot_options_on_curves_tab(self, axes, curve):
        self.plot_options_dialog = PlotConfigDialogPresenter(self.canvas.figure, parent=self.window)
        self.plot_options_dialog.configure_curves_tab(axes, curve)

    def launch_plot_help(self):
        PlotHelpPages.show_help_page_for_figure(self.canvas.figure)

    def copy_to_clipboard(self):
        """Copy the current figure image to clipboard"""
        # store the image in a buffer using savefig(), this has the
        # advantage of applying all the default savefig parameters
        # such as background color; those would be ignored if you simply
        # grab the canvas using Qt
        buf = io.BytesIO()
        self.canvas.figure.savefig(buf)
        QApplication.clipboard().setImage(QImage.fromData(buf.getvalue()))
        buf.close()

    def grid_toggle(self, on):
        """Toggle grid lines on/off"""
        canvas = self.canvas
        axes = canvas.figure.get_axes()
        for ax in axes:
            if type(ax) == Axes:
                # Colorbar
                continue
            elif isinstance(ax, Axes3D):
                # The grid toggle function for 3D plots doesn't let you choose between major and minor lines.
                ax.grid(on)
            else:
                which = 'both' if hasattr(
                    ax, 'show_minor_gridlines') and ax.show_minor_gridlines else 'major'
                ax.grid(on, which=which)
        canvas.draw_idle()

    def fit_toggle(self):
        """Toggle fit browser and tool on/off"""
        if self.fit_browser.isVisible():
            self.fit_browser.hide()
            self.toolbar._actions["toggle_fit"].setChecked(False)
        else:
            if self.toolbar._actions["toggle_superplot"].isChecked():
                self._superplot_hide()
            self.fit_browser.show()

    def _superplot_show(self):
        """Show the superplot"""
        self.superplot = Superplot(self.canvas, self.window)
        if not self.superplot.is_valid():
            logger.warning("Superplot cannot be opened on data not linked "
                           "to a workspace.")
            self.superplot = None
            self.toolbar._actions["toggle_superplot"].setChecked(False)
        else:
            self.window.addDockWidget(Qt.LeftDockWidgetArea,
                                      self.superplot.get_side_view())
            self.window.addDockWidget(Qt.BottomDockWidgetArea,
                                      self.superplot.get_bottom_view())
            self.toolbar._actions["toggle_superplot"].setChecked(True)
            self.superplot.get_bottom_view().setFocus()

    def _superplot_hide(self):
        """Hide the superplot"""
        if self.superplot is None:
            return
        self.window.removeDockWidget(self.superplot.get_side_view())
        self.window.removeDockWidget(self.superplot.get_bottom_view())
        self.superplot.close()
        self.superplot = None
        self.toolbar._actions["toggle_superplot"].setChecked(False)

    def superplot_toggle(self):
        """Toggle superplot dockwidgets on/off"""
        if self.superplot:
            self._superplot_hide()
        else:
            if self.toolbar._actions["toggle_fit"].isChecked():
                self.fit_toggle()
            self._superplot_show()

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
                        if hasattr(ax, 'original_data_surface'):
                            ax.collections[0]._vec = copy.deepcopy(ax.original_data_surface)
                        elif hasattr(ax, 'original_data_wireframe'):
                            ax.collections[0].set_segments(copy.deepcopy(ax.original_data_wireframe))
                        else:
                            ax.view_init()
                    elif ax.images:
                        axesfunctions.update_colorplot_datalimits(ax, ax.images)
                        continue

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

        self.canvas.draw()

    def launch_waterfall_offset_options(self):
        WaterfallPlotOffsetDialogPresenter(self.canvas.figure, parent=self.window)

    def launch_waterfall_fill_area_options(self):
        WaterfallPlotFillAreaDialogPresenter(self.canvas.figure, parent=self.window)

    def update_toolbar_waterfall_plot(self, is_waterfall):
        self.toolbar.set_waterfall_options_enabled(is_waterfall)
        self.toolbar.set_fit_enabled(not is_waterfall)
        self.toolbar.set_generate_plot_script_enabled(not is_waterfall)

    def change_line_collection_colour(self, colour):
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
    """Create a new manager from a num & figure """

    def _new_figure_manager_given_figure_impl(num: int, figure):
        """Create a new figure manager instance for the given figure.
        Forces all public and non-dunder method calls onto the QApplication thread.
        """
        canvas = MantidFigureCanvas(figure)
        return force_method_calls_to_qapp_thread(FigureManagerWorkbench(canvas, num))

    # figure manager & canvas must be created on the QApplication thread
    return QAppThreadCall(_new_figure_manager_given_figure_impl)(int(num), figure)
