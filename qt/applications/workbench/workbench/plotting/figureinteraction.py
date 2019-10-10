# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
"""
Defines interaction behaviour for plotting.
"""
from __future__ import (absolute_import, unicode_literals)

# std imports
from collections import OrderedDict
from copy import copy
from functools import partial
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QActionGroup, QMenu, QApplication

# third party imports
from mantid.api import AnalysisDataService as ads
from mantid.plots import MantidAxes
from mantid.plots.utility import zoom
from mantid.py3compat import iteritems
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.plotting.markers import SingleMarker
from workbench.plotting.figureerrorsmanager import FigureErrorsManager
from workbench.plotting.propertiesdialog import (LabelEditor, XAxisEditor, YAxisEditor,
                                                 SingleMarkerEditor, GlobalMarkerEditor,
                                                 ColorbarAxisEditor)
from workbench.plotting.toolbar import ToolbarStateManager

# Map canvas context-menu string labels to a pair of matplotlib scale-type strings
AXES_SCALE_MENU_OPTS = OrderedDict(
    [("Lin x/Lin y", ("linear", "linear")), ("Log x/Log y", ("log", "log")),
     ("Lin x/Log y", ("linear", "log")), ("Log x/Lin y", ("log", "linear"))])
VALID_LINE_STYLE = ['solid', 'dashed', 'dotted', 'dashdot']
VALID_COLORS = {
    'blue': '#1f77b4',
    'orange': '#ff7f0e',
    'green': '#2ca02c',
    'red': '#d62728',
    'purple': '#9467bd'
}


class FigureInteraction(object):
    """
    Defines the behaviour of interaction events on a figure canvas. Note that
    this currently only works with Qt canvas types.
    """

    def __init__(self, fig_manager):
        """
        Registers handlers for events of interest
        :param fig_manager: A reference to the figure manager containing the
        canvas that receives the events
        """
        # Check it looks like a FigureCanvasQT
        if not hasattr(fig_manager.canvas, "buttond"):
            raise RuntimeError("Figure canvas does not look like a Qt canvas.")

        canvas = fig_manager.canvas
        self._cids = []
        self._cids.append(canvas.mpl_connect('button_press_event', self.on_mouse_button_press))
        self._cids.append(canvas.mpl_connect('button_release_event', self.on_mouse_button_release))
        self._cids.append(canvas.mpl_connect('draw_event', self.draw_callback))
        self._cids.append(canvas.mpl_connect('motion_notify_event', self.motion_event))
        self._cids.append(canvas.mpl_connect('resize_event', self.mpl_redraw_annotations))
        self._cids.append(canvas.mpl_connect('figure_leave_event', self.on_leave))
        self._cids.append(canvas.mpl_connect('axis_leave_event', self.on_leave))
        self._cids.append(canvas.mpl_connect('scroll_event', self.on_scroll))

        self.canvas = canvas
        self.toolbar_manager = ToolbarStateManager(self.canvas.toolbar)
        self.toolbar_manager.home_button_connect(self.redraw_annotations)
        self.fit_browser = fig_manager.fit_browser
        self.errors_manager = FigureErrorsManager(self.canvas)
        self.markers = []
        self.valid_lines = VALID_LINE_STYLE
        self.valid_colors = VALID_COLORS
        self.default_marker_name = 'marker'

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
        if not getattr(event, 'inaxes', None):
            return
        zoom_factor = 1.05 + abs(event.step)/6
        if event.button == 'up':  # zoom in
            zoom(event.inaxes, event.xdata, event.ydata, factor=zoom_factor)
        elif event.button == 'down':  # zoom out
            zoom(event.inaxes, event.xdata, event.ydata, factor=1/zoom_factor)
        event.canvas.draw()

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
            for marker in self.markers:
                marker.remove_all_annotations()
        elif event.button == 1 and marker_selected is not None:
            for marker in marker_selected:
                if len(marker_selected) > 1:
                    marker.set_move_cursor(Qt.ClosedHandCursor, x_pos, y_pos)
                marker.mouse_move_start(x_pos, y_pos)

        if (event.button == canvas.buttond.get(Qt.RightButton)
                and not self.toolbar_manager.is_tool_active()):
            if not marker_selected:
                self._show_context_menu(event)
            else:
                self._show_markers_menu(marker_selected, event)
        elif event.dblclick and event.button == canvas.buttond.get(Qt.LeftButton):
            if not marker_selected:
                self._show_axis_editor(event)
            elif len(marker_selected) == 1:
                self._edit_marker(marker_selected[0])
        elif (self.toolbar_manager.is_zoom_active()
              and event.button == canvas.buttond.get(Qt.MiddleButton)):
            self.toolbar_manager.emit_home_clicked()

    def on_mouse_button_release(self, event):
        """ Stop moving the markers when the mouse button is released """
        if self.toolbar_manager.is_tool_active():
            for marker in self.markers:
                marker.add_all_annotations()
        else:
            x_pos = event.xdata
            y_pos = event.ydata
            self._set_hover_cursor(x_pos, y_pos)

            self.stop_markers(x_pos, y_pos)

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

    def _show_axis_editor(self, event):
        # We assume this is used for editing axis information e.g. labels
        # which are outside of the axes so event.inaxes is no use.
        canvas = self.canvas
        figure = canvas.figure
        axes = figure.get_axes()

        def move_and_show(editor):
            editor.move(QCursor.pos())
            editor.exec_()

        for ax in axes:
            if ax.title.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.title))
            elif ax.xaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.xaxis.label))
            elif ax.yaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.yaxis.label))
            elif (ax.xaxis.contains(event)[0]
                  or any(tick.contains(event)[0] for tick in ax.get_xticklabels())):
                move_and_show(XAxisEditor(canvas, ax))
            elif (ax.yaxis.contains(event)[0]
                  or any(tick.contains(event)[0] for tick in ax.get_yticklabels())):
                if ax == axes[0]:
                    move_and_show(YAxisEditor(canvas, ax))
                else:
                    move_and_show(ColorbarAxisEditor(canvas, ax))

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
        if fig_type == FigureType.Empty or fig_type == FigureType.Image:
            # Fitting or changing scale types does not make sense in
            # these cases
            return

        menu = QMenu()

        if self.fit_browser.tool is not None:
            self.fit_browser.add_to_menu(menu)
            menu.addSeparator()
        self._add_axes_scale_menu(menu, event.inaxes)
        if isinstance(event.inaxes, MantidAxes):
            self._add_normalization_option_menu(menu, event.inaxes)
        self.errors_manager.add_error_bars_menu(menu, event.inaxes)
        self._add_marker_option_menu(menu, event)

        menu.exec_(QCursor.pos())

    def _add_axes_scale_menu(self, menu, ax):
        """Add the Axes scale options menu to the given menu"""
        axes_menu = QMenu("Axes", menu)
        axes_actions = QActionGroup(axes_menu)
        current_scale_types = (ax.get_xscale(), ax.get_yscale())
        for label, scale_types in iteritems(AXES_SCALE_MENU_OPTS):
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
        none_action = norm_menu.addAction('None', lambda: self._set_normalization_none(ax))
        norm_action = norm_menu.addAction('Bin Width',
                                          lambda: self._set_normalization_bin_width(ax))
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
        horizontal = marker_menu.addAction(
            "Horizontal", lambda: self._add_horizontal_marker(event.ydata, y0, y1, event.inaxes))
        vertical = marker_menu.addAction(
            "Vertical", lambda: self._add_vertical_marker(event.xdata, x0, x1, event.inaxes))
        edit = marker_menu.addAction("Edit", lambda: self._global_edit_markers())

        for action in [horizontal, vertical, edit]:
            marker_action_group.addAction(action)

        menu.addMenu(marker_menu)

    def _global_edit_markers(self):
        """Open a window that allows editing of all currently plotted markers"""

        def move_and_show(editor):
            editor.move(QCursor.pos())
            editor.exec_()

        move_and_show(
            GlobalMarkerEditor(self.canvas, self.markers, self.valid_lines, self.valid_colors))

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
                word1, word2 = marker.name.split(' ')
                if word1 == self.default_marker_name:
                    used_numbers.append(int(word2))
            except ValueError:
                continue
        proposed_number = 0
        while True:
            if proposed_number not in used_numbers:
                return "{} {}".format(self.default_marker_name, proposed_number)
            proposed_number += 1

    def _add_horizontal_marker(self,
                               y_pos,
                               lower,
                               upper,
                               axis,
                               name=None,
                               line_style='dashed',
                               color=VALID_COLORS['green']):
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
        marker = SingleMarker(
            self.canvas,
            color,
            y_pos,
            lower,
            upper,
            name=name,
            marker_type='YSingle',
            line_style=line_style,
            axis=axis)
        marker.add_name()
        marker.redraw()
        self.markers.append(marker)

    def _add_vertical_marker(self,
                             x_pos,
                             lower,
                             upper,
                             axis,
                             name=None,
                             line_style='dashed',
                             color=VALID_COLORS['green']):
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
        marker = SingleMarker(
            self.canvas,
            color,
            x_pos,
            lower,
            upper,
            name=name,
            marker_type='XSingle',
            line_style=line_style,
            axis=axis)
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
        move_and_show(
            SingleMarkerEditor(self.canvas, marker, self.valid_lines, self.valid_colors,
                               used_names))

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
        """ This is called at every canvas draw. Redraw the markers. """
        for marker in self.markers:
            marker.redraw()

    def motion_event(self, event):
        """ Move the marker if the mouse is moving and in range """
        if self.toolbar_manager.is_tool_active() or event is None:
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

    def mpl_redraw_annotations(self, event):
        """Redraws all annotations when a mouse button was clicked"""
        if hasattr(event, 'button') and event.button is not None:
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

    def _toggle_normalization(self, ax):
        is_normalized = self._is_normalized(ax)
        for arg_set in ax.creation_args:
            if arg_set['workspaces'] in ax.tracked_workspaces:
                workspace = ads.retrieve(arg_set['workspaces'])
                arg_set['distribution'] = is_normalized
                arg_set_copy = copy(arg_set)
                [
                    arg_set_copy.pop(key)
                    for key in ['function', 'workspaces', 'autoscale_on_update']
                    if key in arg_set_copy.keys()
                ]
                if 'specNum' not in arg_set:
                    if 'wkspIndex' in arg_set:
                        arg_set['specNum'] = workspace.getSpectrum(
                            arg_set.pop('wkspIndex')).getSpectrumNo()
                    else:
                        raise RuntimeError("No spectrum number associated with plot of "
                                           "workspace '{}'".format(workspace.name()))
                for ws_artist in ax.tracked_workspaces[workspace.name()]:
                    if ws_artist.spec_num == arg_set.get('specNum'):
                        ws_artist.is_normalized = not is_normalized
                        ws_artist.replace_data(workspace, arg_set_copy)
        ax.relim()
        ax.autoscale()
        self.canvas.draw()

    def _can_toggle_normalization(self, ax):
        """
        Return True if no plotted workspaces are distributions and all curves
        on the figure are either distributions or non-distributions. Return
        False otherwise.
        :param ax: A MantidAxes object
        :return: bool
        """
        plotted_normalized = []
        for workspace_name, artists in ax.tracked_workspaces.items():
            if not ads.retrieve(workspace_name).isDistribution():
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
        ax.relim()
        ax.set_xscale(scale_types[0])
        ax.set_yscale(scale_types[1])

        self.canvas.draw_idle()
