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
    quickEditSignal = QtCore.Signal(object)
    rmSubplotSignal = QtCore.Signal(object)

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
        self.plotObjects = {}
        grid.addWidget(self.canvas, 1, 0)
        self.setLayout(grid)

    """ this is called when the zoom
    or pan are used. We want to send a
    signal to update the axis ranges """

    def draw_event_callback(self, event):
        self.figure.tight_layout()
        for subplot in self.plotObjects.keys():
            self.emit_subplot_range(subplot)

    def add_annotate(self, subplotName, label):
        if subplotName not in self._context.subplots.keys():
            return
        self._context.add_annotate(subplotName, label)
        self.canvas.draw()

    # todo: add color suppport
    def add_vline(self, subplotName, xvalue, name, color):
        if subplotName not in self._context.subplots.keys():
            return
        self._context.add_vline(subplotName, xvalue, name, color)
        self.canvas.draw()

    def rm_annotate(self, subplotName, name):
        if subplotName not in self._context.subplots.keys():
            return
        self._context.removeLabel(subplotName, name)
        self.canvas.draw()

    def rm_vline(self, subplotName, name):
        if subplotName not in self._context.subplots.keys():
            return
        self._context.removeVLine(subplotName, name)
        self.canvas.draw()

    # plot a workspace, if a new subplot create it.
    def plot(self, subplotName, workspace, specNum=1):
        new = False
        if subplotName not in self._context.subplots.keys():
            self.add_subplot(subplotName, len(list(self.plotObjects.keys())))
            new = True
        self._add_plotted_line(subplotName, workspace, specNum=specNum)
        if new:
            self.emit_subplot_range(subplotName)

    def change_errors(self, state, subplotNames):
        for subplotName in subplotNames:
            self._context.subplots[subplotName].change_errors(state)
            self.canvas.draw()

    # adds plotted line to context and updates GUI
    def _add_plotted_line(self, subplotName, workspace, specNum):
        """ Appends plotted lines to the related subplot list. """
        self._context.addLine(subplotName, workspace, specNum)
        self.canvas.draw()

    def add_subplot(self, subplotName, number):
        self._context.update_gridspec(number + 1)
        gridspec = self._context.gridspec
        self.plotObjects[subplotName] = self.figure.add_subplot(
            gridspec[number], label=subplotName, projection ='mantid')
        self.plotObjects[subplotName].set_title(subplotName)
        self._context.addSubplot(subplotName, self.plotObjects[subplotName])
        self._update()

    def _update(self):
        self._context.update_layout(self.figure)
        self.canvas.draw()

    def emit_subplot_range(self, subplotName):
        self.quickEditSignal.emit(subplotName)
        self._context.subplots[subplotName].redraw_annotations()

    def set_plot_x_range(self, subplotNames, range):
        for subplotName in subplotNames:
            # make a set method in context and set it there
            self.plotObjects[subplotName].set_xlim(range)
            self._context.subplots[subplotName].redraw_annotations()
            self.canvas.draw()

    def set_plot_y_range(self, subplotNames, y_range):
        for subplotName in subplotNames:
            self.plotObjects[subplotName].set_ylim(y_range)
            self._context.subplots[subplotName].redraw_annotations()
            self.canvas.draw()

    def connect_quick_edit_signal(self, slot):
        self.quickEditSignal.connect(slot)

    def disconnect_quick_edit_signal(self):
        self.quickEditSignal.disconnect()

    def connect_rm_subplot_signal(self, slot):
        self.rmSubplotSignal.connect(slot)

    def disconnect_rm_subplot_signal(self):
        self.rmSubplotSignal.disconnect()

    def set_y_autoscale(self, subplotNames, state):
        for subplotName in subplotNames:
            self._context.subplots[subplotName].change_auto(state)
            self.canvas.draw()

    def _rm(self):
        names = list(self._context.subplots.keys())
        # if the remove window is not visable
        if self._rm_window is not None:
            self._raise_rm_window()
        # if the selector is not visable
        elif self._selector_window is not None:
            self._raise_selector_window()
        # if only one subplot just skip selector
        elif len(names) == 1:
            self._get_rm_window(names[0])
        # if no selector and no remove window -> let user pick which subplot to
        # change
        else:
            self._selector_window = self._createSelectWindow(names)
            self._selector_window.subplotSelectorSignal.connect(
                self._get_rm_window)
            self._selector_window.closeEventSignal.connect(
                self._close_selector_window)
            self._selector_window.setMinimumSize(300, 100)
            self._selector_window.show()

    def _rm_subplot(self):
        names = list(self._context.subplots.keys())
        # if the selector is not visable
        if self._selector_window is not None:
            self._raise_selector_window()
        # if no selector and no remove window -> let user pick which subplot to
        # change
        else:
            self._selector_window = self._createSelectWindow(names)
            self._selector_window.subplotSelectorSignal.connect(
                self._remove_subplot)
            self._selector_window.subplotSelectorSignal.connect(
                self._close_selector_window)
            self._selector_window.closeEventSignal.connect(
                self._close_selector_window)
            self._selector_window.setMinimumSize(300, 100)
            self._selector_window.show()

    def _createSelectWindow(self, names):
        return SelectSubplot(names)

    def _raise_rm_window(self):
        self._rm_window.raise_()

    def _raise_selector_window(self):
        self._selector_window.raise_()

    def _close_selector_window(self):
        if self._selector_window is not None:
            self._selector_window.close
            self._selector_window = None

    def _create_rm_window(self, subplotName):
        line_names = list(self._context.subplots[subplotName].lines.keys())
        vline_names = self._context.subplots[subplotName].vlines
        return RemovePlotWindow(lines=line_names, vlines=vline_names, subplot=subplotName, parent=self)

    def _get_rm_window(self, subplotName):
        # always close selector after making a selection
        self._close_selector_window()
        # create the remove window
        self._rm_window = self._create_rm_window(subplotName=subplotName)
        self._rm_window.applyRemoveSignal.connect(self._applyRm)
        self._rm_window.closeEventSignal.connect(self._close_rm_window)
        self._rm_window.setMinimumSize(200, 200)
        self._rm_window.show()

    def _applyRm(self, names):
        remove_subplot = True
        # remove the lines from the subplot
        for name in names:
            if self._rm_window.getState(name):
                self._context.subplots[
                    self._rm_window.subplot].removeLine(name)
            else:
                remove_subplot = False
        # if all of the lines have been removed -> delete subplot
        if remove_subplot:
            self._remove_subplot(self._rm_window.subplot)

        self.canvas.draw()
        # if no subplots then close plotting window
        if len(self._context.subplots.keys()) == 0:
            self._close_rm_window()
            # close plot window once auto grid done
        else:
            self._close_rm_window()

    def _close_rm_window(self):
        self._rm_window.close
        self._rm_window = None

    def _remove_subplot(self, subplotName):
        self.figure.delaxes(self.plotObjects[subplotName])
        del self.plotObjects[subplotName]
        self._context.delete(subplotName)
        self._context.update_gridspec(len(list(self.plotObjects.keys())))
        self._update()
        self.rmSubplotSignal.emit(subplotName)

    def _rm_ws_from_plots(self, workspace_name):
        keys = deepcopy(self._context.subplots.keys())
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
