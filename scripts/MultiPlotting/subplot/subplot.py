# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore

from copy import deepcopy

from matplotlib.figure import Figure
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas

from MultiPlotting.navigation_toolbar import myToolbar
from MultiPlotting.edit_windows.remove_plot_window import RemovePlotWindow
from MultiPlotting.edit_windows.select_subplot import SelectSubplot
from MultiPlotting.subplot.subplot_ADS_observer import SubplotADSObserver

# use this to manage lines and workspaces directly

# visualises multiple plots


class subplot(QtWidgets.QWidget):
    signal_quick_edit = QtCore.Signal(object)
    signal_rm_subplot = QtCore.Signal(object)
    signal_rm_line = QtCore.Signal(object)

    def __init__(self, context):
        super(subplot, self).__init__()
        self._context = context
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        self._rm_window = None
        self._selector_window = None
        # update quick edit from tool bar
        self.canvas.mpl_connect("draw_event", self.draw_event_callback)

        self._ADSObserver = SubplotADSObserver(self)

        grid = QtWidgets.QGridLayout()
        # add toolbar
        self.toolbar = myToolbar(self.canvas, self)
        self.toolbar.update()
        grid.addWidget(self.toolbar, 0, 0)
        self.toolbar.setRmConnection(self._rm)
        self.toolbar.setRmSubplotConnection(self._rm_subplot)
        # add plot
        self.plot_objects = {}
        grid.addWidget(self.canvas, 1, 0)
        self.setLayout(grid)

    """ this is called when the zoom
    or pan are used. We want to send a
    signal to update the axis ranges """
    def draw_event_callback(self, event):
        self.figure.tight_layout()
        for subplot in self.plot_objects.keys():
            self.emit_subplot_range(subplot)

    def add_annotate(self, subplotName, label):
        if subplotName not in self._context.subplots.keys():
            return
        self._context.add_annotate(subplotName, label)
        self.canvas.draw()

    def add_vline(self, subplot_name, xvalue, name, color):
        if subplot_name not in self._context.subplots.keys():
            return
        self._context.add_vline(subplot_name, xvalue, name, color)
        self.canvas.draw()

    def rm_annotate(self, subplot_name, name):
        if subplot_name not in self._context.subplots.keys():
            return
        self._context.removeLabel(subplot_name, name)
        self.canvas.draw()

    def rm_vline(self, subplot_name, name):
        if subplot_name not in self._context.subplots.keys():
            return
        self._context.removeVLine(subplot_name, name)
        self.canvas.draw()

    # plot a workspace, if a new subplot create it.
    def plot(self, subplot_name, workspace, color=None, spec_num=1):
        new = False
        if subplot_name not in self._context.subplots.keys():
            self.add_subplot(subplot_name, len(list(self.plot_objects.keys())))
            new = True
        self._add_plotted_line(subplot_name, workspace, spec_num=spec_num, color=color)
        if new:
            self.emit_subplot_range(subplot_name)

    def change_errors(self, state, subplot_names):
        for subplotName in subplot_names:
            self._context.subplots[subplotName].change_errors(state)
            self.canvas.draw()

    # adds plotted line to context and updates GUI
    def _add_plotted_line(self, subplot_name, workspace, spec_num, color=None):
        """ Appends plotted lines to the related subplot list. """
        self._context.addLine(subplot_name, workspace, spec_num, color=color)
        self.canvas.draw()

    def add_subplot(self, subplot_name, number):
        self._context.update_gridspec(number + 1)
        gridspec = self._context.gridspec
        self.plot_objects[subplot_name] = self.figure.add_subplot(gridspec[number],
                                                                  label=subplot_name,
                                                                  projection='mantid')
        self.plot_objects[subplot_name].set_title(subplot_name)
        self._context.addSubplot(subplot_name, self.plot_objects[subplot_name])
        self._update()

    def _update(self):
        self._context.update_layout(self.figure)
        self.canvas.draw()

    def emit_subplot_range(self, subplot_name):
        self.signal_quick_edit.emit(subplot_name)
        self._context.subplots[subplot_name].redraw_annotations()

    def set_plot_x_range(self, subplot_names, range):
        for subplotName in subplot_names:
            # make a set method in context and set it there
            self.plot_objects[subplotName].set_xlim(range)
            self._context.subplots[subplotName].redraw_annotations()
            self.canvas.draw()

    def set_plot_y_range(self, subplot_names, y_range):
        for subplotName in subplot_names:
            self.plot_objects[subplotName].set_ylim(y_range)
            self._context.subplots[subplotName].redraw_annotations()
            self.canvas.draw()

    def connect_quick_edit_signal(self, slot):
        self.signal_quick_edit.connect(slot)

    def disconnect_quick_edit_signal(self):
        self.signal_quick_edit.disconnect()

    def connect_rm_subplot_signal(self, slot):
        self.signal_rm_subplot.connect(slot)

    def disconnect_rm_subplot_signal(self):
        self.signal_rm_subplot.disconnect()

    def connect_rm_line_signal(self, slot):
        self.signal_rm_line.connect(slot)

    def disconnect_rm_line_signal(self):
        self.signal_rm_line.disconnect()

    def set_y_autoscale(self, subplot_names, state):
        for subplotName in subplot_names:
            self._context.subplots[subplotName].change_auto(state)
            self.canvas.draw()

    def _rm(self):
        names = list(self._context.subplots.keys())
        if len(names) == 1:
            if self._rm_window is not None:
                self._rm_window.show()
            else:
                self._get_rm_window(names[0])
        else:
            if self._rm_window is not None:
                self._rm_window.close()
                self._rm_window = None
            self._close_selector_window()

            self._selector_window = self._create_select_window(names)
            self._selector_window.subplotSelectorSignal.connect(self._get_rm_window)
            self._selector_window.closeEventSignal.connect(self._close_selector_window)
            self._selector_window.setMinimumSize(300, 100)
            self._selector_window.show()

    def _rm_subplot(self):
        names = list(self._context.subplots.keys())
        # If the selector is hidden then close it
        self._close_selector_window()

        self._selector_window = self._create_select_window(names)
        self._selector_window.subplotSelectorSignal.connect(self._remove_subplot)
        self._selector_window.subplotSelectorSignal.connect(self._close_selector_window)
        self._selector_window.closeEventSignal.connect(self._close_selector_window)
        self._selector_window.setMinimumSize(300, 100)
        self._selector_window.show()

    def _create_select_window(self, names):
        return SelectSubplot(names)

    def _close_selector_window(self):
        if self._selector_window is not None:
            self._selector_window.close()
            self._selector_window = None

    def _create_rm_window(self, subplot_name):
        line_names = list(self._context.subplots[subplot_name].lines.keys())
        vline_names = self._context.subplots[subplot_name].vlines
        return RemovePlotWindow(lines=line_names,
                                vlines=vline_names,
                                subplot=subplot_name,
                                parent=self)

    def _get_rm_window(self, subplot_name):
        # always close selector after making a selection
        self._close_selector_window()
        # create the remove window
        self._rm_window = self._create_rm_window(subplot_name=subplot_name)
        self._rm_window.applyRemoveSignal.connect(self._apply_rm)
        self._rm_window.closeEventSignal.connect(self._close_rm_window)
        self._rm_window.setMinimumSize(200, 200)
        self._rm_window.show()

    def remove_lines(self, subplot_name, line_names):
        # remove the lines from the subplot
        for name in line_names:
            self._context.subplots[subplot_name].removeLine(name)

        self.signal_rm_line.emit([str(name) for name in line_names])

        # if all of the lines have been removed -> delete subplot
        if not self._context.get_lines(subplot_name):
            self._remove_subplot(subplot_name)
        else:
            self.canvas.draw()

    def _apply_rm(self, line_names):
        to_close = []
        for name in line_names:
            if self._rm_window.getState(name):
                to_close.append(name)

        self.remove_lines(self._rm_window.subplot, to_close)
        self._close_rm_window()

    def _close_rm_window(self):
        self._rm_window.close
        self._rm_window = None

    def _remove_subplot(self, subplot_name):
        self.figure.delaxes(self.plot_objects[subplot_name])
        del self.plot_objects[subplot_name]
        self._context.delete(subplot_name)
        self._context.update_gridspec(len(list(self.plot_objects.keys())))
        self._update()
        self.signal_rm_subplot.emit(subplot_name)

    def _rm_ws_from_plots(self, workspace_name):
        keys = deepcopy(list(self._context.subplots.keys()))
        for subplot in keys:
            labels = self._context.get_lines_from_WS(subplot, workspace_name)
            for label in labels:
                self._context.removePlotLine(subplot, label)
                self.canvas.draw()
            if self._context.is_subplot_empty(subplot):
                self._remove_subplot(subplot)

    def _replaced_ws(self, workspace):
        for subplot in self._context.subplots.keys():
            redraw = self._context.subplots[subplot].replace_ws(workspace)
            if redraw:
                self.canvas.draw()
