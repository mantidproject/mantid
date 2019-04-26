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
from functools import partial
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QActionGroup, QMenu

# third party imports
from mantid.py3compat import iteritems
from mantidqt.plotting.figuretype import FigureType, figure_type
from workbench.plotting.toolbar import ToolbarStateManager
# local imports
from .propertiesdialog import LabelEditor, XAxisEditor, YAxisEditor

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

    MPL_NOLEGEND = "_nolegend_"

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

        # self.MPL_NOLEGEND = self.canvas.figure.axes[0].MPL_NOLEGEND

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
        if (event.button == canvas.buttond[Qt.RightButton] and
                not self.toolbar_manager.is_tool_active()):
            self._show_context_menu(event)
        elif event.dblclick and event.button == canvas.buttond[Qt.LeftButton]:
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
            elif ax.xaxis.contains(event)[0]:
                move_and_show(XAxisEditor(canvas, ax))
            elif ax.yaxis.contains(event)[0]:
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
        self._add_error_bars_menu(menu)
        menu.exec_(QCursor.pos())

    def _toggle_all_error_bars(self):
        containers = self.canvas.figure.axes[0].containers

        # iterate over all error containers to
        # toggle error bar on lines that have errors already
        for container in containers:
            # extract the line reference from the container
            line = container[0]

            if line.get_label() == self.MPL_NOLEGEND:
                self._toggle_error_bar_for(self._find_errorbar_container(line))

        lines = self.canvas.figure.axes[0].lines
        # Iterate over all lines to add new error bars.
        # Each added errorbar changes the lines list, making the iteration not just 1..n.
        # The line is always removed from pos `index`,
        # and errors are added at the end of the list, the next element is moved 1 position up.
        # Therefore the index should only be increased
        # if the line it is looking for already has errors plotted
        index = 0
        while index < len(lines):
            line = lines[index]
            # line doesn't have errors, add them
            # this will remove the line from the current index,
            # and move the next one to the current index
            if line.get_label() != self.MPL_NOLEGEND:
                self._add_errorbar_for(index)
            else:
                # the line has errors, move forwards in the iteration
                index += 1

    def _update_plot_after(self, func, *args, **kwargs):
        """
        Updates the legend and the plot after the function has been executed.
        Used to funnel through the updates through a common place

        :param func:
        :param args:
        :param kwargs:
        :return:
        """
        func(*args, **kwargs)
        self.canvas.figure.axes[0].legend().draggable()
        self.canvas.draw()

    def _add_errorbar_for(self, index):
        # more magic here
        # TODO extract into class
        from mantid.simpleapi import mtd
        ws = mtd[self.canvas.figure.axes[0].creation_args[index]["workspaces"]]
        specNum = self.canvas.figure.axes[0].creation_args[index]["specNum"]
        # just plots the spectrum with errors on the same plot
        errorbar_container = self.canvas.figure.axes[0].errorbar(ws, specNum=specNum)

        lines = self.canvas.figure.axes[0].lines
        # change the color of the data line
        errorbar_container[0].set_color(lines[index].get_color())
        # change the color of the error line
        errorbar_container[2][0].set_color(lines[index].get_color())

        # The swaps are done do pretend the line was replaced 'inplace'
        # this keeps the legend order the same.

        # swap in .lines, remove the old line reference
        lines[index], lines[-1] = lines[-1], lines[index]
        # delete the reference to the old line
        del lines[-1]

        # swap in .creation_args, remove the old args reference
        cargs = self.canvas.figure.axes[0].creation_args
        cargs[index], cargs[-1] = cargs[-1], cargs[index]
        # delete the reference to the old creation args
        del cargs[-1]

    def _toggle_error_bar_for(self, errorbar_container):
        # the container has the errors there
        error_line = errorbar_container.lines[2][0]
        error_line.set_visible(not error_line.get_visible())

    def _find_errorbar_container(self, line):
        containers = self.canvas.figure.axes[0].containers

        for container in containers:
            if line == container[0]:
                return container

    def _add_error_bars_menu(self, menu):
        lines = self.canvas.figure.axes[0].lines

        # if there's more than one line plotted, then
        # add a sub menu, containing an action to hide the
        # error bar for each line
        if len(lines) > 1:
            # add into the correct menu,
            # if there is a single line, then
            # add the action to the main menu
            error_bars_menu = QMenu("Error Bars", menu)
            menu.addMenu(error_bars_menu)

            for index, line in enumerate(lines):
                if line.get_label() == self.MPL_NOLEGEND:
                    # this line has an errorbar, which contains the label in the legend
                    error_line = self._find_errorbar_container(line)
                    label = error_line.get_label()
                    # simply toggles the visibility of the error line
                    error_bars_menu.addAction(label,
                                              partial(self._update_plot_after, self._toggle_error_bar_for, error_line))
                else:
                    # this line has no errorbar, so the label is contained here
                    label = line.get_label()
                    error_bars_menu.addAction(label, partial(self._update_plot_after, self._add_errorbar_for, index))

        # always add the toggle all error bars option
        menu.addAction("Toggle error bars", partial(self._update_plot_after, self._toggle_all_error_bars))

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
