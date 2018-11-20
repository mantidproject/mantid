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
        # add some dummy plot
        self.plots = subPlot("test", self._context)
        self.plots.connect_quick_edit_signal(self.update_quick_edit)
        # add dummy line
        self.plots.plot("test","first line", self._context.ws)

        # create GUI layout
        layout.addWidget(self.plots)
        layout.addWidget(self.quickEdit.widget)
        self.setLayout(layout)

    def errors_changed(self,state):
        print("waaa ", state)
        # get subplot selected....
        self.plots.change_errors(state, "test")

    def x_range_changed(self,range):
        self.plots.set_plot_x_range("test", range)

    def y_range_changed(self,range):
        self.plots.set_plot_y_range("test", range)

    def update_quick_edit(self):
        self.quickEdit.loadFromContext()


#    def getPresenter(self):
#        return self._presenter

#    @property
#    def widget(self):
#        return self._presenter.widget

#    def updateLabel(self, message):
#        self._presenter.updateLabel(message)

    # interact with context
#    def updateContext(self):
#        self._presenter.updateContext()

 #   def loadFromContext(self, context):
        # extract relevant info from context via model
 #       model = self._presenter.model
 #       sub_context = model.getSubContext()
 #       # update the view with the subcontext
 #       view = self._presenter.widget
 #       view.loadFromContext(sub_context)
