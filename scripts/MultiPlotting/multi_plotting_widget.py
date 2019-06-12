# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore
from copy import deepcopy
from MultiPlotting.subplot.subplot import subplot
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget
from MultiPlotting.multi_plotting_context import *
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

from mantidqt.plotting.ads_observer import FigureManagerADSObserver


class MultiPlotWindow(object):
    def __init__(self, window_title="plotting", close_callback=None):
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        self.close_callback = close_callback

        self.plot_context = PlottingContext()
        self.multi_plot = MultiPlotWidget(window_title, self.plot_context, self.canvas)
        self.window = self.multi_plot
        self.window.closeSignal.connect(self.close_callback)

        self._ADSObserver = FigureManagerADSObserver(self)

    def show(self):
        self.window.show()

    def raise_(self):
        self.window.raise_()

    def emit_close(self):
        self.window.close()


class MultiPlotWidget(QtWidgets.QMainWindow):
    closeSignal = QtCore.Signal()

    def __init__(self, window_title, context, canvas, parent=None):
        super(MultiPlotWidget, self).__init__(parent=parent)
        self._context = context
        layout = QtWidgets.QVBoxLayout()
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        self.quickEdit = QuickEditWidget(self)
        self.quickEdit.connect_x_range_changed(self._x_range_changed)
        self.quickEdit.connect_y_range_changed(self._y_range_changed)
        self.quickEdit.connect_errors_changed(self._errors_changed)
        self.quickEdit.connect_autoscale_changed(self._autoscale_changed)
        self.quickEdit.connect_plot_selection(self._selection_changed)

        # add some dummy plot
        self.plots = subplot(self._context, canvas)
        self.plots.connect_quick_edit_signal(self._update_quick_edit)
        self.plots.connect_rm_subplot_signal(self._update_quick_edit)
        # create GUI layout
        splitter.addWidget(self.plots)
        splitter.addWidget(self.quickEdit.widget)
        layout.addWidget(splitter)
        central_widget = QtWidgets.QWidget()
        central_widget.setLayout(layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle(window_title)

        self.plots.set_remove_line_connection(self.handle_remove_workspace)

        self.plots.set_remove_subplot_connection(self.handle_remove_subplot)

        self.plots.set_remove_line_connection(self.handle_remove_workspace)

        self.plots.set_remove_subplot_connection(self.handle_remove_subplot)

    """ plotting """

    def add_subplot(self, name):
        self.plots.add_subplot(name, len(self.quickEdit.get_selection()))

        self.quickEdit.add_subplot(name)

    def plot(self, subplotName, ws, specNum=1):
        self.plots.plot(subplotName, ws, specNum=specNum)

    def remove_subplot(self, name):
        self.plots._remove_subplot(name)

    def get_subplots(self):
        return list(self._context.subplots.keys())

    def add_vline_and_annotate(self, subplotName, xvalue, label):
        self.add_annotate(subplotName, label)
        self.add_vline(subplotName, xvalue, label.text)

    def rm_vline_and_annotate(self, subplotName, name):
        self.rm_annotate(subplotName, name)
        self.rm_vline(subplotName, name)

    def add_annotate(self, subplotName, label):
        self.plots.add_annotate(subplotName, label)

    def add_vline(self, subplotName, xvalue, name):
        self.plots.add_vline(subplotName, xvalue, name)

    def rm_annotate(self, subplotName, name):
        self.plots.rm_annotate(subplotName, name)

    def rm_vline(self, subplotName, name):
        self.plots.rm_vline(subplotName, name)

    # gets inital values for quickEdit
    def set_all_values(self):
        names = self.quickEdit.get_selection()
        xrange = list(self._context.subplots[names[0]].xbounds)
        yrange = list(self._context.subplots[names[0]].ybounds)
        for name in names:
            xbounds = self._context.subplots[name].xbounds
            ybounds = self._context.subplots[name].ybounds
            if xrange[0] > xbounds[0]:
                xrange[0] = deepcopy(xbounds[0])
            if xrange[1] < xbounds[1]:
                xrange[1] = deepcopy(xbounds[1])
            if yrange[0] > ybounds[0]:
                yrange[0] = deepcopy(ybounds[0])
            if yrange[1] < ybounds[1]:
                yrange[1] = deepcopy(ybounds[1])
        self._context.set_xBounds(xrange)
        self._context.set_yBounds(yrange)
        self._x_range_changed(xrange)
        self._y_range_changed(yrange)
        # get tick boxes correct
        errors = self._check_all_errors(names)
        self.quickEdit.set_errors(errors)
        self._change_errors(errors, names)

    def set_all_values_to(self,xrange,yrange):
        self._context.set_xBounds(xrange)
        self._context.set_yBounds(yrange)
        self._x_range_changed(xrange)
        self._y_range_changed(yrange)

    def connectCloseSignal(self, slot):
        self.closeSignal.connect(slot)

    def removeSubplotConnection(self, slot):
        self.plots.connect_rm_subplot_signal(slot)

    def disconnectCloseSignal(self):
        self.closeSignal.disconnect()

    def removeSubplotDisonnect(self):
        self.plots.disconnect_rm_subplot_signal()

    def has_subplot(self, name):
        return name in self._context.subplots.keys()

    """ update GUI """

    def _if_empty_close(self):
        if not self._context.subplots:
            self.close()

    def _update_quick_edit(self, subplotName):
        names = self.quickEdit.get_selection()
        if subplotName not in self._context.subplots.keys():
            self.quickEdit.rm_subplot(subplotName)
            self._if_empty_close()
            return
        xrange = self._context.subplots[subplotName].xbounds
        yrange = self._context.subplots[subplotName].ybounds
        if len(names) == 0:
            return
        # if all selected update everyone
        if len(names) > 1:
            self.quickEdit.set_plot_x_range(xrange)
            self.quickEdit.set_plot_y_range(yrange)
        # if changed current selection
        elif names[0] == subplotName:
            self.quickEdit.set_plot_x_range(xrange)
            self.quickEdit.set_plot_y_range(yrange)
        # if a different plot changed
        else:
            pass

    def _selection_changed(self, index):
        names = self.quickEdit.get_selection()
        xrange = self._context.get_xBounds()
        yrange = self._context.get_yBounds()
        errors = True
        if len(names) == 1:
            xrange = self._context.subplots[names[0]].xbounds
            yrange = self._context.subplots[names[0]].ybounds
            errors = self._context.subplots[names[0]].errors
        else:
            errors = self._check_all_errors(names)
        # update values
        self.quickEdit.set_errors(errors)
        self._change_errors(errors, names)
        self.quickEdit.set_plot_x_range(xrange)
        self.quickEdit.set_plot_y_range(yrange)

        # force update of plots if selection is all
        if len(names) > 1:
            self._x_range_changed(xrange)
            self._y_range_changed(yrange)

    def _autoscale_changed(self, state):
        names = self.quickEdit.get_selection()
        self.plots.set_y_autoscale(names, True)

    def _change_errors(self, state, names):
        self.plots.change_errors(state, names)

    def _errors_changed(self, state):
        names = self.quickEdit.get_selection()
        self._change_errors(state, names)

    def _x_range_changed(self, xRange):
        names = self.quickEdit.get_selection()
        if len(names) > 1:
            self._context.set_xBounds(xRange)
        self.plots.set_plot_x_range(names, xRange)
        self.quickEdit.set_plot_x_range(xRange)

    def _y_range_changed(self, yRange):
        names = self.quickEdit.get_selection()
        if len(names) > 1:
            self._context.set_yBounds(yRange)
        self.plots.set_plot_y_range(names, yRange)
        self.quickEdit.set_plot_y_range(yRange)

    def _check_all_errors(self, names):
        for name in names:
            if self._context.subplots[name].errors is False:
                return False
        return True

    def emit_close(self):
        self.close()

    def handle_remove_workspace(self):
        self.plots._rm()
        self._if_empty_close()

    def handle_remove_subplot(self):
        self.plots._rm_subplot()
        self._if_empty_close()

    def closeEvent(self, event):
        self.closeSignal.emit()
        super(MultiPlotWidget, self).closeEvent(event)
