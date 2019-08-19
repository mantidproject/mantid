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
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid.plots import MantidAxes
from mantid.py3compat import iteritems
from mantidqt.plotting.figuretype import FigureType, figure_type
from mantidqt.plotting.markers import SingleMarker
from workbench.plotting.figureerrorsmanager import FigureErrorsManager
from workbench.plotting.propertiesdialog import LabelEditor, XAxisEditor, YAxisEditor, SingleMarkerEditor
from workbench.plotting.toolbar import ToolbarStateManager

# Map canvas context-menu string labels to a pair of matplotlib scale-type strings
AXES_SCALE_MENU_OPTS = OrderedDict([
    ("Lin x/Lin y", ("linear", "linear")),
    ("Log x/Log y", ("log", "log")),
    ("Lin x/Log y", ("linear", "log")),
    ("Log x/Lin y", ("log", "linear"))]
)


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
        self._cids.append(canvas.mpl_connect('resize_event', self.redraw_annotations))

        self.canvas = canvas
        self.toolbar_manager = ToolbarStateManager(self.canvas.toolbar)
        self.fit_browser = fig_manager.fit_browser
        self.errors_manager = FigureErrorsManager(self.canvas)
        self.markers = []
        self.vertical_markers = []
        self.valid_lines = [str(name) for name in ['solid', 'dashed', 'dotted', 'dashdot']]

    @property
    def nevents(self):
        return len(self._cids)

    def disconnect(self):
        """
        Disconnects all registered event handers
        """
        for id in self._cids:
            self.canvas.mpl_disconnect(id)

    # ------------------------ Handlers --------------------
    def on_mouse_button_press(self, event):
        """Respond to a MouseEvent where a button was pressed"""
        # local variables to avoid constant self lookup
        canvas = self.canvas
        x_pos = event.xdata
        y_pos = event.ydata
        if x_pos is not None and y_pos is not None:
            marker_selected = [marker for marker in self.markers if marker.is_above(x_pos, y_pos)]
        else:
            marker_selected = []

        if (event.button == canvas.buttond.get(Qt.RightButton) and
                not self.toolbar_manager.is_tool_active()):
            if not marker_selected:
                self._show_context_menu(event)
            else:
                self._show_markers_menu(marker_selected, event)
        elif event.dblclick and event.button == canvas.buttond.get(Qt.LeftButton):
            if not marker_selected:
                self._show_axis_editor(event)
            else:
                for marker in marker_selected:
                    self._edit_marker(marker)
                marker_selected = None

        # If left button clicked, start moving peaks
        if self.toolbar_manager.is_tool_active():
            for marker in self.markers:
                marker.remove_all_annotations()
        elif event.button == 1 and marker_selected is not None:
            change_cursor = False
            for marker in self.markers:
                if event.xdata is None or event.ydata is None:
                    continue
                marker.mouse_move_start(event.xdata, event.ydata)
                change_cursor = marker.is_marker_moving() or change_cursor
            if change_cursor:
                QApplication.setOverrideCursor(QCursor(Qt.ClosedHandCursor))

    def on_mouse_button_release(self, event):
        """ Stop moving the markers when the mouse button is released """
        if self.toolbar_manager.is_tool_active():
            for marker in self.markers:
                marker.add_all_annotations()
        else:
            for marker in self.markers:
                if marker.is_marker_moving():
                    marker.add_name()
                marker.mouse_move_stop()
            QApplication.restoreOverrideCursor()

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
            elif (ax.xaxis.contains(event)[0] or
                  any(tick.contains(event)[0] for tick in ax.get_xticklabels())):
                move_and_show(XAxisEditor(canvas, ax))
            elif (ax.yaxis.contains(event)[0] or
                  any(tick.contains(event)[0] for tick in ax.get_yticklabels())):
                move_and_show(YAxisEditor(canvas, ax))

    def _show_markers_menu(self, markers, event):
        if not event.inaxes:
            return

        fig_type = figure_type(self.canvas.figure)
        if fig_type == FigureType.Empty or fig_type == FigureType.Image:
            return

        menu = QMenu()
        QApplication.restoreOverrideCursor()

        for marker in markers:
            self._single_marker_menu(menu, marker)

        menu.exec_(QCursor.pos())

    def _single_marker_menu(self, menu, marker):
        marker_menu = QMenu(marker.name, menu)
        marker_action_group = QActionGroup(marker_menu)

        delete = marker_menu.addAction("Delete", lambda: self._delete_marker(marker))
        edit = marker_menu.addAction("Edit", lambda: self._edit_marker(marker))

        for action in [delete, edit]:
            marker_action_group.addAction(action)
            action.setCheckable(True)
            action.setChecked(False)

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
        none_action = norm_menu.addAction(
            'None', lambda: self._set_normalization_none(ax))
        norm_action = norm_menu.addAction(
            'Bin Width', lambda: self._set_normalization_bin_width(ax))
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
        marker_menu = QMenu("Markers", menu)
        marker_action_group = QActionGroup(marker_menu)
        x0, x1 = event.inaxes.get_xlim()
        y0, y1 = event.inaxes.get_ylim()
        horizontal = marker_menu.addAction("Horizontal", lambda: self._add_horizontal_marker(event.ydata, y0, y1))
        vertical = marker_menu.addAction("Vertical", lambda: self._add_vertical_marker(event.xdata, x0, x1))
        export = marker_menu.addAction("Export", lambda: self._export_markers())
        _import = marker_menu.addAction("Import", lambda: self._import_markers(event.inaxes))

        for action in [horizontal, vertical, export, _import]:
            marker_action_group.addAction(action)
            action.setCheckable(True)
            action.setChecked(False)

        menu.addMenu(marker_menu)

    def _export_markers(self):
        marker_horizontal = CreateEmptyTableWorkspace()
        marker_vertical = CreateEmptyTableWorkspace()
        marker_horizontal.addColumn(type='float', name='position')
        marker_horizontal.addColumn(type='str', name='name')
        marker_horizontal.addColumn(type='str', name='line style')
        marker_vertical.addColumn(type='float', name='position')
        marker_vertical.addColumn(type='str', name='name')
        marker_vertical.addColumn(type='str', name='line style')
        for marker in self.markers:
            if marker.marker_type == 'XSingle':
                marker_vertical.addRow([marker.get_position(),
                                        marker.name,
                                        marker.style])
            else:
                marker_horizontal.addRow([marker.get_position(),
                                          marker.name,
                                          marker.style])

    def _import_markers(self, ax, marker_horizontal='marker_horizontal', marker_vertical='marker_vertical'):
        for marker in self.markers:
            marker.remove_all_annotations()
            marker.remove()
        marker_horizontal = ads.retrieve(marker_horizontal)
        marker_vertical = ads.retrieve(marker_vertical)
        x0, x1 = ax.get_xlim()
        y0, y1 = ax.get_ylim()

        for i in range(marker_horizontal.rowCount()):
            row = marker_horizontal.row(i)
            position = row['position']
            name = row['name']
            style = row['line style']
            self._add_horizontal_marker(position, x0, x1, name, style)

        for i in range(marker_vertical.rowCount()):
            row = marker_vertical.row(i)
            position = row['position']
            name = row['name']
            style = row['line style']
            self._add_vertical_marker(position, y0, y1, name, style)

    def _get_free_marker_name(self):
        used_numbers = []
        for marker in self.markers:
            try:
                used_numbers.append(int(marker.name.split(' ')[1]))
            except IndexError:
                continue
        proposed_number = 0
        while True:
            if proposed_number not in used_numbers:
                return "marker {}".format(proposed_number)
            proposed_number += 1

    def _add_horizontal_marker(self, y_pos, lower, upper, name=None, line_style='dashed'):
        if name is None:
            name = self._get_free_marker_name()
        marker = SingleMarker(self.canvas, 'C2', y_pos, lower, upper, name=name,
                              marker_type='YSingle', line_style=line_style)
        marker.add_name()
        marker.redraw()
        self.markers.append(marker)

    def _add_vertical_marker(self, x_pos, lower, upper, name=None, line_style='dashed'):
        if name is None:
            name = self._get_free_marker_name()
        marker = SingleMarker(self.canvas, 'C2', x_pos, lower, upper, name=name,
                              marker_type='XSingle', line_style=line_style)
        marker.add_name()
        marker.redraw()
        self.markers.append(marker)

    def _delete_marker(self, marker):
        if marker in self.markers:
            marker.remove_all_annotations()
            self.markers.remove(marker)
            self.canvas.draw()

    def _edit_marker(self, marker):
        QApplication.restoreOverrideCursor()

        def move_and_show(editor):
            editor.move(QCursor.pos())
            editor.exec_()

        move_and_show(SingleMarkerEditor(self.canvas, marker, self.valid_lines))

    def draw_callback(self, _):
        """ This is called at every canvas draw. Redraw the markers. """
        for marker in self.markers:
            marker.redraw()

    def motion_event(self, event):
        if self.toolbar_manager.is_tool_active():
            return

        """ Move the marker if the mouse is moving and in range """
        x = event.xdata
        y = event.ydata
        if x is None or y is None:
            return

        if not any([marker.is_marker_moving() for marker in self.markers]):
            if any([marker.is_above(x, y) for marker in self.markers]):
                QApplication.setOverrideCursor(Qt.OpenHandCursor)
            else:
                QApplication.restoreOverrideCursor()

        should_move = False
        for marker in self.markers:
            should_move = marker.mouse_move(x, y) or should_move

        if should_move:
            self.canvas.draw()

    def redraw_annotations(self, event):
        if hasattr(event, 'button') and event.button is not None:
            for marker in self.markers:
                marker.remove_all_annotations()
                marker.add_all_annotations()

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
                [arg_set_copy.pop(key) for key in ['function', 'workspaces', 'autoscale_on_update']
                 if key in arg_set_copy.keys()]
                if 'specNum' not in arg_set:
                    if 'wkspIndex' in arg_set:
                        arg_set['specNum'] = workspace.getSpectrum(
                            arg_set.pop('wkspIndex')).getSpectrumNo()
                    else:
                        raise RuntimeError(
                            "No spectrum number associated with plot of "
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
