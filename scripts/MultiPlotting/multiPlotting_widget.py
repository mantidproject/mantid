# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore
from copy import deepcopy
from MultiPlotting.subplot.subPlot import subPlot
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget


class MultiPlotWidget(QtWidgets.QWidget):

    def __init__(self, context, parent=None):
        super(MultiPlotWidget, self).__init__()
        self._context = context
        layout = QtWidgets.QVBoxLayout()
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        self.quickEdit = QuickEditWidget(self._context, self)
        self.quickEdit.connect_x_range_changed(self._x_range_changed)
        self.quickEdit.connect_y_range_changed(self._y_range_changed)
        self.quickEdit.connect_errors_changed(self._errors_changed)
        self.quickEdit.connect_autoscale_changed(self._autoscale_changed)
        self.quickEdit.connect_plot_selection(self._selection_changed)

        # add some dummy plot
        self.plots = subPlot(self._context)
        self.plots.connect_quick_edit_signal(self._update_quick_edit)

        # create GUI layout
        splitter.addWidget(self.plots)
        splitter.addWidget(self.quickEdit.widget)
        layout.addWidget(splitter)
        self.setLayout(layout)

    """ plotting """

    def add_subplot(self, name, code):
        self.plots.add_subplot(name, code)
        self.quickEdit.add_subplot(name)

    def plot(self, subplotName, ws, specNum=1):
        self.plots.plot(subplotName, ws, specNum=specNum)

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

    """ update GUI """

    def _update_quick_edit(self, subplotName):
        names = self.quickEdit.get_selection()
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
