# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

#from MultiPlotting.multiPlotting_view import MultiPlotView
#from MultiPlotting.multiPlotting_presenter import MultiPlotPresenter
#from MultiPlotting.multiPlotting_model import MultiPlotModel
from qtpy import QtWidgets, QtCore

from MultiPlotting.subplot.subPlot import subPlot
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget

class MultiPlotWidget(QtWidgets.QWidget):

    def __init__(self, context, parent=None):
        super(MultiPlotWidget,self).__init__()
        self._context = context
        layout = QtWidgets.QVBoxLayout()

        self.quickEdit = QuickEditWidget(self._context,self)
        self.quickEdit.connect_x_range_changed(self.x_range_changed)
        self.quickEdit.connect_y_range_changed(self.y_range_changed)
        self.quickEdit.connect_errors_changed(self.errors_changed)
        self.quickEdit.connect_autoscale_changed(self.autoscale_changed)
        self.quickEdit.connect_plot_selection(self.selection_changed)

        # add some dummy plot
        self.plots = subPlot(self._context)
        self.plots.connect_quick_edit_signal(self.update_quick_edit)

        # create GUI layout
        layout.addWidget(self.plots)
        layout.addWidget(self.quickEdit.widget)
        self.setLayout(layout)

    def selection_changed(self,index):
        names = self.quickEdit.get_selection()
        # make this a get method and do something clever
        # to make sure the full data set is shown (min/max across all plots)
        xrange = self._context.get_xBounds()
        yrange = self._context.get_yBounds()
        if len(names) == 1:
           xrange = self._context.subplots[names[0]].xbounds
           yrange = self._context.subplots[names[0]].ybounds
        self.quickEdit.set_plot_x_range(xrange)
        self.quickEdit.set_plot_y_range(yrange)

    def add_subplot(self, name,code):
        self.plots.add_subplot(name,code)
        self.quickEdit.add_subplot(name)

    def plot(self,subplotName,ws,specNum=1):
        self.plots.plot(subplotName, ws,specNum)

    def autoscale_changed(self,state):
        names = self.quickEdit.get_selection()
        self.plots.set_y_autoscale(names,state)

    def errors_changed(self,state):
        names = self.quickEdit.get_selection()
        self.plots.change_errors(state, names)

    def x_range_changed(self,range):
        names = self.quickEdit.get_selection()
        self.plots.set_plot_x_range(names, range)
        self.quickEdit.set_plot_x_range(range)

    def y_range_changed(self,range):
        names = self.quickEdit.get_selection()     
        self.plots.set_plot_y_range(names, range)

    def update_quick_edit(self):
        # need to check if its correct subplot
        self.quickEdit.loadFromContext()
        subplotNames = self.quickEdit.get_selection()
        range = self.quickEdit.get_y_bounds()
        for subplotName in subplotNames:
            # need to do an autoscale y to record max/min values when errors on/off or add a line
            # will then need to use the correct limits to set the y range correctly. 
            if self.plots.plotObjects[subplotName].get_ybound() != range[0]:
               self.quickEdit.set_y_autoscale(False)
               self.quickEdit.set_plot_y_range(range)



