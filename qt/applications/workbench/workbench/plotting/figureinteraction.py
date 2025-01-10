# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
"""
Defines interaction behaviour for plotting.
"""

# std imports
import numpy as np
from contextlib import contextmanager
from collections import OrderedDict
from copy import copy
from functools import partial

# third party imports
from matplotlib.container import ErrorbarContainer
from matplotlib.contour import QuadContourSet
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QActionGroup, QMenu, QApplication, QAction
from matplotlib.colors import LogNorm, Normalize
from matplotlib.collections import Collection
from mpl_toolkits.mplot3d.axes3d import Axes3D

# mantid imports
from mantid.api import AnalysisDataService as ads
from mantid.plots import datafunctions, MantidAxes, axesfunctions, MantidAxes3D, LegendProperties
from mantid.plots.utility import zoom, MantidAxType, legend_set_draggable
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.plotting.markers import SingleMarker
from mantidqt.widgets.plotconfigdialog.curvestabwidget import curve_has_errors, CurveProperties
from workbench.plotting.figureerrorsmanager import FigureErrorsManager
from workbench.plotting.propertiesdialog import (
    LabelEditor,
    XAxisEditor,
    YAxisEditor,
    SingleMarkerEditor,
    GlobalMarkerEditor,
    ColorbarAxisEditor,
    ZAxisEditor,
    LegendEditor,
)
from workbench.plotting.style import VALID_LINE_STYLE, VALID_COLORS
from workbench.plotting.toolbar import ToolbarStateManager

# Map canvas context-menu string labels to a pair of matplotlib scale-type strings
AXES_SCALE_MENU_OPTS = OrderedDict(
    [
        ("Lin x/Lin y", ("linear", "linear")),
        ("Log x/Log y", ("log", "log")),
        ("Lin x/Log y", ("linear", "log")),
        ("Log x/Lin y", ("log", "linear")),
    ]
)
COLORBAR_SCALE_MENU_OPTS = OrderedDict([("Linear", Normalize), ("Log", LogNorm)])


@contextmanager
def errorbar_caps_removed(ax):
    # Error bar caps are considered lines so they are removed before checking the number of lines on the axes so
    # they aren't confused for "actual" lines.
    error_bar_caps = datafunctions.remove_and_return_errorbar_cap_lines(ax)
    yield
    # Re-add error bar caps
    for cap in error_bar_caps:
        ax.add_line(cap)


class FigureInteraction(object):
    """
    Defines the behaviour of interaction events on a figure canvas. Note that
    this currently only works with Qt canvas types.
    """

    ERROR_BARS_MENU_TEXT = "Error Bars"
    SHOW_ERROR_BARS_BUTTON_TEXT = "Show all errors"
    HIDE_ERROR_BARS_BUTTON_TEXT = "Hide all errors"

    def __init__(self, fig_manager):
        """
        Registers handlers for events of interest
        :param fig_manager: A reference to the figure manager containing the
        canvas that receives the events
        """
        self.fig_manager = fig_manager
        # Check it looks like a FigureCanvasQT
        if not hasattr(self.fig_manager.canvas, "buttond"):
            raise RuntimeError("Figure canvas does not look like a Qt canvas.")

        canvas = self.fig_manager.canvas
        self._cids = []
        self._cids.append(canvas.mpl_connect("button_press_event", self.on_mouse_button_press))
        self._cids.append(canvas.mpl_connect("button_release_event", self.on_mouse_button_release))
        self._cids.append(canvas.mpl_connect("draw_event", self.draw_callback))
        self._cids.append(canvas.mpl_connect("motion_notify_event", self.motion_event))
        self._cids.append(canvas.mpl_connect("resize_event", self.mpl_redraw_annotations))
        self._cids.append(canvas.mpl_connect("figure_leave_event", self.on_leave))
        self._cids.append(canvas.mpl_connect("scroll_event", self.on_scroll))
        self._cids.append(canvas.mpl_connect("key_press_event", self.on_key_press))

        self.canvas = canvas
        self.toolbar_manager = ToolbarStateManager(self.canvas.toolbar)
        self.toolbar_manager.home_button_connect(self.redraw_annotations)
        self.fit_browser = self.fig_manager.fit_browser
        self.errors_manager = FigureErrorsManager(self.canvas)
        self.markers = []
        self.valid_lines = VALID_LINE_STYLE
        self.valid_colors = VALID_COLORS
        self.default_marker_name = "marker"
        self.double_click_event = None
        self.marker_selected_in_double_click_event = None

    @property
    def nevents(self):
        return len(self._cids)

    def disconnect(self):
        """
        Disconnects all registered event handlers
        """
        for cid in self._cids:
            self.canvas.mpl_disconnect(cid)

    # ------------------------ Handlers --------------------
    def on_scroll(self, event):
        """Respond to scroll events: zoom in/out"""
        self.canvas.toolbar.push_current()
        if (
            not getattr(event, "inaxes", None)
            or isinstance(event.inaxes, Axes3D)
            or len(event.inaxes.images) == 0
            and len(event.inaxes.lines) == 0
        ):
            return

        self._correct_for_scroll_event_on_legend(event)

        zoom_factor = 1.05 + abs(event.step) / 6
        if event.button == "up":  # zoom in
            zoom(event.inaxes, event.xdata, event.ydata, factor=zoom_factor)
        elif event.button == "down":  # zoom out
            zoom(event.inaxes, event.xdata, event.ydata, factor=1 / zoom_factor)
        self.redraw_annotations()
        event.canvas.draw()

    def on_key_press(self, event):
        ax = event.inaxes
        if ax is None or isinstance(ax, Axes3D) or len(ax.get_images()) == 0 and len(ax.get_lines()) == 0:
            return

        if event.key == "k":
            current_xscale = ax.get_xscale()
            next_xscale = self._get_next_axis_scale(current_xscale)
            self._quick_change_axes((next_xscale, ax.get_yscale()), ax)

        if event.key == "l":
            current_yscale = ax.get_yscale()
            next_yscale = self._get_next_axis_scale(current_yscale)
            self._quick_change_axes((ax.get_xscale(), next_yscale), ax)

    def _get_next_axis_scale(self, current_scale):
        if current_scale == "linear":
            return "log"
        return "linear"

    def _correct_for_scroll_event_on_legend(self, event):
        # Corrects default behaviour in Matplotlib where legend is picked up by scroll event
        legend = event.inaxes.axes.get_legend()
        if legend is not None and legend.get_draggable() and legend.contains(event):
            legend_set_draggable(legend, False)
            legend_set_draggable(legend, True)

    def on_mouse_button_press(self, event):
        """Respond to a MouseEvent where a button was pressed"""
        # local variables to avoid constant self lookup
        canvas = self.canvas
        x_pos = event.xdata
        y_pos = event.ydata
        self._set_hover_cursor(x_pos, y_pos)
        if x_pos is not None and y_pos is not None:
            marker_selected = [marker for marker in self.markers if marker.is_above(x_pos, y_pos)]
        else:
            marker_selected = []

        # If left button clicked, start moving peaks
        if self.toolbar_manager.is_tool_active():
            self._remove_all_marker_annotations()
        elif event.button == 1 and marker_selected is not None:
            for marker in marker_selected:
                if len(marker_selected) > 1:
                    marker.set_move_cursor(Qt.ClosedHandCursor, x_pos, y_pos)
                marker.mouse_move_start(x_pos, y_pos)

        if event.button == canvas.buttond.get(Qt.RightButton) and not self.toolbar_manager.is_tool_active():
            if not marker_selected:
                self._show_context_menu(event)
            else:
                self._show_markers_menu(marker_selected, event)
        elif event.dblclick and event.button == canvas.buttond.get(Qt.LeftButton):
            # Double-clicking will open a dialog depending on where the mouse event is located. However, if the dialog
            # is opened here, mpl will not process the mouse release event after the dialog closes, and will not end any
            # panning/zooming/rotating drag events that are in progress. Therefore, we store the event data to use in
            # the mouse release callback.
            self.double_click_event = event
            self.marker_selected_in_double_click_event = marker_selected

        elif event.button == canvas.buttond.get(Qt.MiddleButton):
            if self.toolbar_manager.is_zoom_active():
                self.toolbar_manager.emit_sig_home_clicked()
            # Activate pan on middle button press
            elif not self.toolbar_manager.is_tool_active():
                if event.inaxes and event.inaxes.can_pan():
                    event.button = 1
                    try:
                        self._remove_all_marker_annotations()
                        self.canvas.toolbar.press_pan(event)
                    finally:
                        event.button = 3
        elif isinstance(event.inaxes, Axes3D):
            event.inaxes._button_press(event)

    def on_mouse_button_release(self, event):
        """Stop moving the markers when the mouse button is released"""
        # Release pan on middle button release
        if event.button == self.canvas.buttond.get(Qt.MiddleButton):
            if not self.toolbar_manager.is_tool_active():
                event.button = 1
                try:
                    self.canvas.toolbar.release_pan(event)
                    self._add_all_marker_annotations()
                finally:
                    event.button = 3
        elif event.button == self.canvas.buttond.get(Qt.RightButton) and self.toolbar_manager.is_zoom_active():
            # Reset the axes limits if you right click while using the zoom tool.
            self.toolbar_manager.emit_sig_home_clicked()

        if self.toolbar_manager.is_tool_active():
            self._add_all_marker_annotations()
        else:
            x_pos = event.xdata
            y_pos = event.ydata
            self._set_hover_cursor(x_pos, y_pos)

            self.stop_markers(x_pos, y_pos)

        # If the mouse is released after a double click event, check whether we need to open a settings dialog.
        if self.double_click_event:
            self._open_double_click_dialog(self.double_click_event, self.marker_selected_in_double_click_event)
            self.marker_selected_in_double_click_event = None
            self.double_click_event = None

        self.legend_bounds_check(event)

    @staticmethod
    def legend_bounds_check(event):
        fig = event.canvas.figure

        for ax in fig.get_axes():
            legend1 = ax.get_legend()
            if legend1 is None:
                continue

            bbox_fig = fig.get_window_extent()
            bbox_legend = legend1.get_window_extent()

            outside_window = (
                bbox_legend.x1 < bbox_fig.x0 or bbox_legend.x0 > bbox_fig.x1 or bbox_legend.y1 < bbox_fig.y0 or bbox_legend.y0 > bbox_fig.y1
            )

            # Snap back legend
            if outside_window:
                props = LegendProperties.from_legend(legend1)
                LegendProperties.create_legend(props, legend1.axes)
                fig.canvas.draw_idle()

    def on_leave(self, event):
        """
        When leaving the axis or canvas, restore cursor to default one
        and stop moving the markers
        """
        QApplication.restoreOverrideCursor()
        if event:
            self.stop_markers(event.xdata, event.ydata)

    def set_all_markers_visible(self, visible):
        for marker in self.markers:
            marker.set_visible(visible)

    def stop_markers(self, x_pos, y_pos):
        """
        Stop all markers that are moving and draw the annotations
        """
        if x_pos is None or y_pos is None:
            return

        for marker in self.markers:
            if marker.is_marker_moving():
                marker.set_move_cursor(None, x_pos, y_pos)
                marker.add_all_annotations()
            marker.mouse_move_stop()

    def _open_double_click_dialog(self, event, marker_selected=None):
        """
        Opens a settings dialog based on the event location.
        @param event: the object representing the double click event
        """
        if not marker_selected:
            if not self._show_axis_editor(event):
                # Don't run inside 3D+ plots, as there is already matplotlib behaviour here.
                if not hasattr(event.inaxes, "zaxis"):
                    self._show_plot_options(event)
        elif len(marker_selected) == 1:
            self._edit_marker(marker_selected[0])

    def _show_axis_editor(self, event):
        """
        Decides whether to show a dialog to edit axis information based on the contents of the
        event. Shows a dialog if necessary.
        @param event: the object representing the event
        @return: a flag to denote whether an action was taken e.g. opening a dialog.
        """
        # We assume this is used for editing axis information e.g. labels
        # which are outside of the axes so event.inaxes is no use.
        canvas = self.canvas
        figure = canvas.figure
        axes = figure.get_axes()
        action_taken = False

        def move_and_show(editor):
            nonlocal action_taken
            action_taken = True
            editor.move(QCursor.pos())
            editor.exec_()

        for ax in axes:
            if ax.title.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.title))
            elif ax.xaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.xaxis.label))
            elif ax.yaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.yaxis.label))
            elif ax.xaxis.contains(event)[0] or any(tick.contains(event)[0] for tick in ax.get_xticklabels()):
                move_and_show(XAxisEditor(canvas, ax))
            elif ax.yaxis.contains(event)[0] or any(tick.contains(event)[0] for tick in ax.get_yticklabels()):
                if "colorbar" in ax._label:
                    move_and_show(ColorbarAxisEditor(canvas, ax))
                else:
                    move_and_show(YAxisEditor(canvas, ax))
            elif hasattr(ax, "zaxis"):
                if ax.zaxis.label.contains(event)[0]:
                    move_and_show(LabelEditor(canvas, ax.zaxis.label))
                elif ax.zaxis.contains(event)[0] or any(tick.contains(event)[0] for tick in ax.get_zticklabels()):
                    move_and_show(ZAxisEditor(canvas, ax))
            elif ax.get_legend() is not None and ax.get_legend().contains(event)[0]:
                # We have to set the legend as non-draggable else we hold onto the legend
                # until the mouse button is clicked again
                legend_set_draggable(ax.get_legend(), False)
                legend_texts = ax.get_legend().get_texts()
                active_lines = datafunctions.get_legend_handles(ax)

                remove_legend_flag = True  # remove the legend if no curve texts were clicked
                for legend_text, curve in zip(legend_texts, active_lines):
                    if legend_text.contains(event)[0]:
                        remove_legend_flag = False
                        move_and_show(LegendEditor(canvas, legend_text, curve))
                legend_set_draggable(ax.get_legend(), True)

                if remove_legend_flag:
                    action_taken = True
                    legend = ax.get_legend()
                    legend.set_visible(False)
                    canvas.draw()

        return action_taken

    def _show_plot_options(self, event):
        """
        Opens the plot settings dialog and switches to the curves tab.
        @param event: the object representing the mouse event
        """
        if not event.inaxes:
            return

        axes = event.inaxes
        clicked_curve = None
        for curve in axes.lines:
            if curve.contains(event)[0]:
                clicked_curve = curve
                break

        # Launch with the first curve that contains the event
        self.fig_manager.launch_plot_options_on_curves_tab(axes, clicked_curve)

    def _show_markers_menu(self, markers, event):
        """
        This is opened when right-clicking on top of a marker.
        The menu will have an entry for each marker near the cursor at the time of clicking.
        The menu will allow deletion and single marker editing
        :param markers: list of markers close to the cursor at the time of clicking
        :param event: The MouseEvent that generated this call
        """
        if not event.inaxes:
            return

        QApplication.restoreOverrideCursor()
        fig_type = figure_type(self.canvas.figure)
        if fig_type == FigureType.Empty or fig_type == FigureType.Image:
            return

        menu = QMenu()

        for marker in markers:
            self._single_marker_menu(menu, marker)

        menu.exec_(QCursor.pos())

    def _single_marker_menu(self, menu, marker):
        """
        Entry in a menu that allows editing/deleting a single marker
        :param menu: instance of QMenu to add this submenu to
        :param marker: marker to be edited with the menu
        """
        marker_menu = QMenu(marker.name, menu)
        marker_action_group = QActionGroup(marker_menu)

        delete = marker_menu.addAction("Delete", lambda: self._delete_marker(marker))
        edit = marker_menu.addAction("Edit", lambda: self._edit_marker(marker))

        for action in [delete, edit]:
            marker_action_group.addAction(action)

        menu.addMenu(marker_menu)

    def _show_context_menu(self, event):
        """Display a relevant context menu on the canvas
        :param event: The MouseEvent that generated this call
        """
        if not event.inaxes:
            # the current context menus are ony relevant for axes
            return

        fig_type = figure_type(self.canvas.figure)
        if fig_type == FigureType.Empty:
            # Fitting or changing scale types does not make sense in this case
            return

        menu = QMenu()

        if fig_type == FigureType.Image or fig_type == FigureType.Contour:
            if isinstance(event.inaxes, MantidAxes):
                self._add_axes_scale_menu(menu, event.inaxes)
                self._add_normalization_option_menu(menu, event.inaxes)
                self._add_colorbar_axes_scale_menu(menu, event.inaxes)
        elif fig_type == FigureType.Surface:
            if isinstance(event.inaxes, MantidAxes3D):
                self._add_colorbar_axes_scale_menu(menu, event.inaxes)
        elif fig_type != FigureType.Wireframe:
            if self.fit_browser.tool is not None:
                self.fit_browser.add_to_menu(menu)
                menu.addSeparator()
            self._add_axes_scale_menu(menu, event.inaxes)
            if isinstance(event.inaxes, MantidAxes):
                self._add_normalization_option_menu(menu, event.inaxes)
            self.add_error_bars_menu(menu, event.inaxes)
            self._add_marker_option_menu(menu, event)
            self._add_plot_type_option_menu(menu, event.inaxes)
            self._add_legend_toggle_action(menu, event)

        menu.exec_(QCursor.pos())

    def _add_axes_scale_menu(self, menu, ax):
        """Add the Axes scale options menu to the given menu"""
        axes_menu = QMenu("Axes", menu)
        axes_actions = QActionGroup(axes_menu)
        current_scale_types = (ax.get_xscale(), ax.get_yscale())
        for label, scale_types in AXES_SCALE_MENU_OPTS.items():
            action = axes_menu.addAction(label, partial(self._quick_change_axes, scale_types, ax))
            if current_scale_types == scale_types:
                action.setCheckable(True)
                action.setChecked(True)
            axes_actions.addAction(action)
        menu.addMenu(axes_menu)

    def _add_normalization_option_menu(self, menu, ax):
        # Check if toggling normalization makes sense
        can_toggle_normalization = self._can_toggle_normalization(ax)
        if not can_toggle_normalization:
            return None

        # Create menu
        norm_menu = QMenu("Normalization", menu)
        norm_actions_group = QActionGroup(norm_menu)
        none_action = norm_menu.addAction("None", lambda: self._set_normalization_none(ax))
        norm_action = norm_menu.addAction("Bin Width", lambda: self._set_normalization_bin_width(ax))
        for action in [none_action, norm_action]:
            norm_actions_group.addAction(action)
            action.setCheckable(True)

        # Update menu state
        is_normalized = self._is_normalized(ax)
        if is_normalized:
            norm_action.setChecked(True)
        else:
            none_action.setChecked(True)

        menu.addMenu(norm_menu)

    def _add_colorbar_axes_scale_menu(self, menu, ax):
        """Add the Axes scale options menu to the given menu"""
        axes_menu = QMenu("Color bar", menu)
        axes_actions = QActionGroup(axes_menu)
        images = ax.get_images() + [col for col in ax.collections if isinstance(col, Collection)]
        for label, scale_type in COLORBAR_SCALE_MENU_OPTS.items():
            action = axes_menu.addAction(label, partial(self._change_colorbar_axes, scale_type))
            if type(images[0].norm) is scale_type:
                action.setCheckable(True)
                action.setChecked(True)
            axes_actions.addAction(action)
        menu.addMenu(axes_menu)

    def add_error_bars_menu(self, menu, ax):
        """
        Add menu actions to toggle the errors for all lines in the plot.

        Lines without errors are added in the context menu first,
        then lines containing errors are appended afterwards.

        This is done so that the context menu always has
        the same order of curves as the legend is currently showing - and the
        legend always appends curves with errors after the lines without errors.
        Relevant source, as of 10 July 2019:
        https://github.com/matplotlib/matplotlib/blob/154922992722db37a9d9c8680682ccc4acf37f8c/lib/matplotlib/legend.py#L1201

        :param menu: The menu to which the actions will be added
        :type menu: QMenu
        :param ax: The Axes containing lines to toggle errors on
        """
        # if the ax is not a MantidAxes, and there are no errors plotted,
        # then do not add any options for the menu
        if not isinstance(ax, MantidAxes) and len(ax.containers) == 0:
            return

        error_bars_menu = QMenu(self.ERROR_BARS_MENU_TEXT, menu)
        error_bars_menu.addAction(
            self.SHOW_ERROR_BARS_BUTTON_TEXT,
            partial(self.errors_manager.update_plot_after, self.errors_manager.toggle_all_errors, ax, make_visible=True),
        )
        error_bars_menu.addAction(
            self.HIDE_ERROR_BARS_BUTTON_TEXT,
            partial(self.errors_manager.update_plot_after, self.errors_manager.toggle_all_errors, ax, make_visible=False),
        )
        menu.addMenu(error_bars_menu)

        self.errors_manager.active_lines = self.errors_manager.get_curves_from_ax(ax)

        # if there's more than one line plotted, then
        # add a sub menu, containing an action to hide the
        # error bar for each line
        error_bars_menu.addSeparator()
        add_later = []
        for index, line in enumerate(self.errors_manager.active_lines):
            if curve_has_errors(line):
                curve_props = CurveProperties.from_curve(line)
                # Add lines without errors first, lines with errors are appended later. Read docstring for more info
                if not isinstance(line, ErrorbarContainer):
                    action = error_bars_menu.addAction(
                        line.get_label(),
                        partial(self.errors_manager.update_plot_after, self.errors_manager.toggle_error_bars_for, ax, line),
                    )
                    action.setCheckable(True)
                    action.setChecked(not curve_props.hide_errors)
                else:
                    add_later.append(
                        (
                            line.get_label(),
                            partial(self.errors_manager.update_plot_after, self.errors_manager.toggle_error_bars_for, ax, line),
                            not curve_props.hide_errors,
                        )
                    )

        for label, function, visible in add_later:
            action = error_bars_menu.addAction(label, function)
            action.setCheckable(True)
            action.setChecked(visible)

    def _add_marker_option_menu(self, menu, event):
        """
        Entry in main context menu to:
         - add horizontal/vertical markers
         - open a marker editor window.
        The editor window allows editing of all currently plotted markers
        :param menu: instance of QMenu to append this submenu to
        :param event: mpl event that generated the call
        """
        marker_menu = QMenu("Markers", menu)
        marker_action_group = QActionGroup(marker_menu)
        x0, x1 = event.inaxes.get_xlim()
        y0, y1 = event.inaxes.get_ylim()
        horizontal = marker_menu.addAction("Horizontal", lambda: self._add_horizontal_marker(event.ydata, y0, y1, event.inaxes))
        vertical = marker_menu.addAction("Vertical", lambda: self._add_vertical_marker(event.xdata, x0, x1, event.inaxes))
        edit = marker_menu.addAction("Edit", lambda: self._global_edit_markers())

        for action in [horizontal, vertical, edit]:
            marker_action_group.addAction(action)

        menu.addMenu(marker_menu)

    def _add_legend_toggle_action(self, menu, event):
        legend = event.inaxes.axes.get_legend()
        legend_action = QAction("Show legend", menu, checkable=True)
        legend_action.setChecked(legend is not None and legend.get_visible())
        legend_action.toggled.connect(lambda: self._toggle_legend_and_redraw(event.inaxes.axes))
        menu.addAction(legend_action)

    def _add_plot_type_option_menu(self, menu, ax):
        with errorbar_caps_removed(ax):
            # Able to change the plot type to waterfall if there is only one axes, it is a MantidAxes, and there is more
            # than one line on the axes.
            if len(ax.get_figure().get_axes()) > 1 or not isinstance(ax, MantidAxes) or len(ax.get_lines()) <= 1:
                return

        plot_type_menu = QMenu("Plot Type", menu)
        plot_type_action_group = QActionGroup(plot_type_menu)
        standard = plot_type_menu.addAction("1D", lambda: self._change_plot_type(ax, plot_type_action_group.checkedAction()))
        waterfall = plot_type_menu.addAction("Waterfall", lambda: self._change_plot_type(ax, plot_type_action_group.checkedAction()))

        for action in [waterfall, standard]:
            plot_type_action_group.addAction(action)
            action.setCheckable(True)

        if ax.is_waterfall():
            waterfall.setChecked(True)
        else:
            standard.setChecked(True)

        menu.addMenu(plot_type_menu)

    def _change_plot_type(self, ax, action):
        if action.text() == "Waterfall":
            ax.set_waterfall(True)
        else:
            ax.set_waterfall(False)

    def _global_edit_markers(self):
        """Open a window that allows editing of all currently plotted markers"""

        def move_and_show(editor):
            editor.move(QCursor.pos())
            editor.exec_()

        move_and_show(GlobalMarkerEditor(self.canvas, self.markers, self.valid_lines, self.valid_colors))

    def _get_free_marker_name(self):
        """
        Generate a unique name for a new marker.
        The name will have the form: "marker n"
        Where n is the lowest positive integer that makes the name unique.
        E.g. suppose there are markers: "marker 0", "marker 2", "marker 3"
             the function will return "marker 1"
        """
        used_numbers = []
        for marker in self.markers:
            try:
                word1, word2 = marker.name.split(" ")
                if word1 == self.default_marker_name:
                    used_numbers.append(int(word2))
            except ValueError:
                continue
        proposed_number = 0
        while True:
            if proposed_number not in used_numbers:
                return "{} {}".format(self.default_marker_name, proposed_number)
            proposed_number += 1

    def _add_horizontal_marker(self, y_pos, lower, upper, axis, name=None, line_style="dashed", color=VALID_COLORS["green"]):
        """
        Add a horizontal marker to the plot and append it to the list of open markers
        :param y_pos: position to plot the marker to
        :param lower: x value to start the marker at
        :param upper: x value to stop the marker at
        :param name: label displayed beside the marker
        :param line_style: 'solid', 'dashed', etc.
        :param color: 'r', 'g', 'b' etc. or some hex code
        """
        if name is None:
            name = self._get_free_marker_name()
        marker = SingleMarker(self.canvas, color, y_pos, lower, upper, name=name, marker_type="YSingle", line_style=line_style, axis=axis)
        marker.add_name()
        marker.redraw()
        self.markers.append(marker)

    def _add_vertical_marker(self, x_pos, lower, upper, axis, name=None, line_style="dashed", color=VALID_COLORS["green"]):
        """
        Add a vertical marker to the plot and append it to the list of open markers
        :param x_pos: position to plot the marker to
        :param lower: y value to start the marker at
        :param upper: y value to stop the marker at
        :param name: label displayed beside the marker
        :param line_style: 'solid', 'dashed', etc.
        :param color: 'r', 'g', 'b' etc. or some hex code
        """
        if name is None:
            name = self._get_free_marker_name()
        marker = SingleMarker(self.canvas, color, x_pos, lower, upper, name=name, marker_type="XSingle", line_style=line_style, axis=axis)
        marker.add_name()
        marker.redraw()
        self.markers.append(marker)

    def _delete_marker(self, marker):
        """
        If marker currently plotted, remove its label and delete the marker.
        """
        if marker in self.markers:
            marker.remove_all_annotations()
            marker.marker.remove()
            self.markers.remove(marker)
            self.canvas.draw()

    def _edit_marker(self, marker):
        """
        Open a dialog window to edit the marker properties (position, name, line style, colour)
        """

        def move_and_show(editor):
            editor.move(QCursor.pos())
            editor.exec_()

        used_names = [str(_marker.name) for _marker in self.markers]
        QApplication.restoreOverrideCursor()
        move_and_show(SingleMarkerEditor(self.canvas, marker, self.valid_lines, self.valid_colors, used_names))

    def _set_hover_cursor(self, x_pos, y_pos):
        """
        If the cursor is above a single marker make it into a pointing hand.
        If the cursor is above multiple markers (eg. an intersection) make it into a cross.
        Otherwise set it to the default one.
        :param x_pos: cursor x position
        :param y_pos: cursor y position
        """
        if x_pos is None or y_pos is None:
            QApplication.restoreOverrideCursor()
            return

        is_moving = any([marker.is_marker_moving() for marker in self.markers])
        hovering_over = sum([1 for marker in self.markers if marker.is_above(x_pos, y_pos)])
        if not is_moving:
            if hovering_over == 1:
                QApplication.setOverrideCursor(Qt.PointingHandCursor)
            elif hovering_over > 1:
                QApplication.setOverrideCursor(Qt.CrossCursor)
            else:
                QApplication.restoreOverrideCursor()

    def draw_callback(self, _):
        """This is called at every canvas draw. Redraw the markers."""
        for marker in self.markers:
            marker.redraw()

    def motion_event(self, event):
        """Move the marker if the mouse is moving and in range"""
        if self.toolbar_manager.is_tool_active() or self.toolbar_manager.is_fit_active() or event is None:
            return

        x = event.xdata
        y = event.ydata
        self._set_hover_cursor(x, y)

        should_move = any([marker.mouse_move(x, y) for marker in self.markers])

        if should_move:
            self.canvas.draw()

    def redraw_annotations(self):
        """Remove all annotations and add them again."""
        for marker in self.markers:
            marker.remove_all_annotations()
            marker.add_all_annotations()

    def _remove_all_marker_annotations(self):
        for marker in self.markers:
            marker.remove_all_annotations()

    def _add_all_marker_annotations(self):
        for marker in self.markers:
            marker.add_all_annotations()

    def mpl_redraw_annotations(self, event):
        """Redraws all annotations when a mouse button was clicked"""
        if hasattr(event, "button") and event.button is not None:
            self.redraw_annotations()

    def _is_normalized(self, ax):
        artists = [art for art in ax.tracked_workspaces.values()]
        return all(art[0].is_normalized for art in artists)

    def _set_normalization_bin_width(self, ax):
        if self._is_normalized(ax):
            return
        self._toggle_normalization(ax)

    def _set_normalization_none(self, ax):
        if not self._is_normalized(ax):
            return
        self._toggle_normalization(ax)

    def _toggle_normalization(self, selected_ax):
        if figure_type(self.canvas.figure) == FigureType.Image and len(self.canvas.figure.get_axes()) > 1:
            axes = datafunctions.get_axes_from_figure(self.canvas.figure)
        else:
            axes = [selected_ax]

        for ax in axes:
            waterfall = isinstance(ax, MantidAxes) and ax.is_waterfall()
            if waterfall:
                x, y = ax.waterfall_x_offset, ax.waterfall_y_offset
                has_fill = ax.waterfall_has_fill()

                if has_fill:
                    line_colour_fill = datafunctions.waterfall_fill_is_line_colour(ax)
                    if line_colour_fill:
                        fill_colour = None
                    else:
                        fill_colour = datafunctions.get_waterfall_fills(ax)[0].get_facecolor()

                ax.update_waterfall(0, 0)

            # The colorbar can get screwed up with ragged workspaces and log scales as they go
            # through the normalisation toggle.
            # Set it to Linear and change it back after if necessary, since there's no reason
            # to duplicate the handling.
            colorbar_log = False
            if ax.images:
                colorbar_log = isinstance(ax.images[-1].norm, LogNorm)
                if colorbar_log:
                    self._change_colorbar_axes(Normalize)

            self._change_plot_normalization(ax)

            if ax.lines:  # Relim causes issues with colour plots, which have no lines.
                ax.relim()
                ax.autoscale()

            if ax.images:  # Colour bar limits are wrong if workspace is ragged. Set them manually.
                colorbar_min = np.nanmin(ax.images[-1].get_array())
                colorbar_max = np.nanmax(ax.images[-1].get_array())
                for image in ax.images:
                    image.set_clim(colorbar_min, colorbar_max)

                    # Update the colorbar label
                    cb = image.colorbar
                    if cb:
                        datafunctions.add_colorbar_label(cb, ax.get_figure().axes)
                if colorbar_log:  # If it had a log scaled colorbar before, put it back.
                    self._change_colorbar_axes(LogNorm)

                axesfunctions.update_colorplot_datalimits(ax, ax.images)

            datafunctions.set_initial_dimensions(ax)
            if waterfall:
                ax.update_waterfall(x, y)

                if has_fill:
                    ax.set_waterfall_fill(True, fill_colour)

        self.canvas.draw()

    def _change_plot_normalization(self, ax):
        is_normalized = self._is_normalized(ax)
        for arg_set in ax.creation_args:
            if arg_set["function"] == "contour":
                continue

            if arg_set["workspaces"] in ax.tracked_workspaces:
                workspace = ads.retrieve(arg_set["workspaces"])
                arg_set["distribution"] = is_normalized
                if "specNum" not in arg_set:
                    if "wkspIndex" in arg_set:
                        arg_set["specNum"] = workspace.getSpectrum(arg_set.pop("wkspIndex")).getSpectrumNo()
                    else:
                        raise RuntimeError("No spectrum number associated with plot of " "workspace '{}'".format(workspace.name()))

                arg_set_copy = copy(arg_set)
                for key in ["function", "workspaces", "autoscale_on_update", "norm"]:
                    try:
                        del arg_set_copy[key]
                    except KeyError:
                        continue
                # 2D plots have no spec number so remove it
                if figure_type(self.canvas.figure) in [FigureType.Image, FigureType.Contour]:
                    arg_set_copy.pop("specNum")
                for ws_artist in ax.tracked_workspaces[workspace.name()]:
                    if ws_artist.spec_num == arg_set_copy.get("specNum"):
                        ws_artist.is_normalized = not is_normalized

                        # This check is to prevent the contour lines being re-plotted using the colorfill plot args.
                        if isinstance(ws_artist._artists[0], QuadContourSet):
                            contour_line_colour = ws_artist._artists[0].get_edgecolor()

                            ws_artist.replace_data(workspace, None)

                            # Re-apply the contour line colour
                            ws_artist._artists[0].set_color(contour_line_colour)
                        else:
                            ws_artist.replace_data(workspace, arg_set_copy)

    def _can_toggle_normalization(self, ax):
        """
        Return True if no plotted workspaces are distributions, all curves
        on the figure are either distributions or non-distributions,
        and the data_replace_cb method was set when plotting . Return
        False otherwise.
        :param ax: A MantidAxes object
        :return: bool
        """
        plotted_normalized = []
        if not ax.creation_args:
            return False
        axis = ax.creation_args[0].get("axis", None)
        for workspace_name, artists in ax.tracked_workspaces.items():
            if hasattr(ads.retrieve(workspace_name), "isDistribution"):
                is_dist = ads.retrieve(workspace_name).isDistribution()
            else:
                is_dist = True
            if axis != MantidAxType.BIN and not is_dist and ax.data_replaced:
                plotted_normalized += [a.is_normalized for a in artists]
            else:
                return False
        if all(plotted_normalized) or not any(plotted_normalized):
            return True
        return False

    def _quick_change_axes(self, scale_types, ax):
        """
        Perform a change of axes on the figure to that given by the option
        :param scale_types: A 2-tuple of strings giving matplotlib axes scale types
        """
        # Recompute the limits of the data. If the scale is set to log
        # on either axis, Fit enabled & then disabled, then the axes are
        # not rescaled properly because the vertical marker artists were
        # included in the last computation of the data limits and
        # set_xscale/set_yscale only autoscale the view
        xlim = copy(ax.get_xlim())
        ylim = copy(ax.get_ylim())
        ax.set_xscale(scale_types[0])
        ax.set_yscale(scale_types[1])
        ax.set_xlim(xlim)
        ax.set_ylim(ylim)

        self.canvas.draw_idle()

    def _change_colorbar_axes(self, scale_type):
        for ax in self.canvas.figure.get_axes():
            images = ax.get_images() + [col for col in ax.collections if isinstance(col, Collection)]
            for image in images:
                if image.norm.vmin is not None and image.norm.vmax is not None:
                    datafunctions.update_colorbar_scale(self.canvas.figure, image, scale_type, image.norm.vmin, image.norm.vmax)

        self.canvas.draw_idle()

    def _toggle_legend_and_redraw(self, ax):
        legend = ax.get_legend()
        if not legend:
            ax.legend()
        else:
            legend.set_visible(not legend.get_visible())
        self.canvas.draw()
