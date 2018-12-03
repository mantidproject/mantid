# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from qtpy import QtWidgets

from MultiPlotting.QuickEdit.quickEdit_view import QuickEditView
from MultiPlotting.QuickEdit.quickEdit_model import QuickEditModel
from MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter

class QuickEditWidget(object):
    def __init__(self,context,parent=None):
        view = QuickEditView(None, parent)
        model = QuickEditModel(context)
        self._presenter = QuickEditPresenter(view,model)

    @property
    def widget(self):
        return self._presenter.widget

    def connect_autoscale_changed(self,slot):
        self._presenter.connect_autoscale_changed(slot)

    def connect_errors_changed(self,slot):
        self._presenter.connect_errors_changed(slot)

    def connect_x_range_changed(self,slot):
        self._presenter.connect_x_range_changed(slot)

    def connect_y_range_changed(self,slot):
        self._presenter.connect_y_range_changed(slot)

    def connect_plot_selection(self,slot):
        self._presenter.connect_plot_selection(slot)

    def loadFromContext(self):
        model = self._presenter.model
        sub_context = model.getSubContext()
        # update the view with the subcontext
        view = self._presenter.widget
        view.loadFromContext(sub_context)

    def add_subplot(self,name):
        self._presenter.add_subplot(name)

    def get_selection(self):
        name = self._presenter.widget.current_selection()
        if name == "All":
           return self._presenter.all()
        return [name] 

    def set_plot_x_range(self,range):
        self._presenter.set_plot_x_range(range)

    def set_plot_y_range(self,range):
        self._presenter.set_plot_y_range(range)

    def set_y_autoscale(self,state):
        self._presenter.set_auto(state)

    def set_errors(self,state):
        self._presenter.set_errors(state)

    def get_y_bounds(self):
        view = self._presenter.widget
        return view.get_y_bounds()