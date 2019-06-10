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

# third party imports
from mantid.py3compat import iteritems
from mantidqt.plotting.figuretype import FigureType, figure_type
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QActionGroup, QMenu

# local imports
from mantid.api import AnalysisDataService as ads
from mantid.plots import MantidAxes
from workbench.plotting.propertiesdialog import LabelEditor, XAxisEditor, YAxisEditor
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
        self._cids.append(canvas.mpl_connect('button_press_event',
                                             self.on_mouse_button_press))

        self.canvas = canvas
        self.toolbar_manager = ToolbarStateManager(self.canvas.toolbar)
        self.fit_browser = fig_manager.fit_browser

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
        if (event.button == canvas.buttond.get(Qt.RightButton) and
                not self.toolbar_manager.is_tool_active()):
            self._show_context_menu(event)
        elif event.dblclick and event.button == canvas.buttond.get(Qt.LeftButton):
            self._show_axis_editor(event)

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
        self._add_axes_scale_menu(menu)
        if isinstance(event.inaxes, MantidAxes):
            self._add_normalization_option_menu(menu, event.inaxes)
        menu.exec_(QCursor.pos())

    def _add_axes_scale_menu(self, menu):
        """Add the Axes scale options menu to the given menu"""
        axes_menu = QMenu("Axes", menu)
        axes_actions = QActionGroup(axes_menu)
        current_scale_types = self._get_axes_scale_types()
        for label, scale_types in iteritems(AXES_SCALE_MENU_OPTS):
            action = axes_menu.addAction(label, partial(self._quick_change_axes, scale_types))
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
            workspace = ads.retrieve(arg_set['workspaces'])
            arg_set['distribution'] = is_normalized
            arg_set_copy = copy(arg_set)
            [arg_set_copy.pop(key) for key in ['function', 'workspaces']]
            if 'specNum' not in arg_set:
                if 'wkspIndex' in arg_set:
                    arg_set['specNum'] = workspace.getSpectrum(
                        arg_set.pop('wkspIndex')).getSpectrumNo()
                else:
                    raise RuntimeError(
                        "No spectrum number associated with plot of workspace "
                        "'{}'".format(workspace.name()))
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

    def _get_axes_scale_types(self):
        """Return a 2-tuple containing the axis scale types if all Axes on the figure are the same
         otherwise we return None. It assumes a figure with atleast 1 Axes object"""
        all_axes = self.canvas.figure.get_axes()
        scale_types = (all_axes[0].get_xscale(), all_axes[0].get_yscale())
        for axes in all_axes[1:]:
            other_scales = (axes.get_xscale(), axes.get_yscale())
            if scale_types != other_scales:
                scale_types = None
                break

        return scale_types

    def _quick_change_axes(self, scale_types):
        """
        Perform a change of axes on the figure to that given by the option
        :param scale_types: A 2-tuple of strings giving matplotlib axes scale types
        """
        for axes in self.canvas.figure.get_axes():
            # Recompute the limits of the data. If the scale is set to log
            # on either axis, Fit enabled & then disabled, then the axes are
            # not rescaled properly because the vertical marker artists were
            # included in the last computation of the data limits and
            # set_xscale/set_yscale only autoscale the view
            axes.relim()
            axes.set_xscale(scale_types[0])
            axes.set_yscale(scale_types[1])

        self.canvas.draw_idle()
