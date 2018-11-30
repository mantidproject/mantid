# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function


class QuickEditPresenter(object):
    def __init__(self,view,model):
        self._view = view
        self._model = model
        self._view.connect_autoscale_changed(self.autoscale_changed)
        self._view.connect_x_range_changed(self.updateContext)

    @property
    def widget(self):
        return self._view
    @property
    def model(self):
        return self._model

    def connect_autoscale_changed(self,slot):
        self._view.connect_autoscale_changed(slot)

    def connect_errors_changed(self,slot):
        self._view.connect_errors_changed(slot)

    def autoscale_changed(self,state):
        self._view.change_autoscale(state)

    def connect_x_range_changed(self,slot):
        self._view.connect_x_range_changed(slot)

    def connect_y_range_changed(self,slot):
        self._view.connect_y_range_changed(slot)
		
    def connect_plot_selection(self,slot):
        self._view.connect_plot_selection(slot)

    def updateContext(self,range):
        subContext = self._view.getSubContext()
        self.model.updateContext(subContext)

    def add_subplot(self,name):
        current = self._view.current_selection()
        self._view.add_subplot(name)
        index = self._view.find_index(current)
        self._view.set_index(index)

    def all(self):
        return [self._view.plot_at_index(index) for index in range(1,self._view.number_of_plots())]

    def set_plot_x_range(self,range):
        self._view.set_plot_x_range(range)
    def set_plot_y_range(self,range):
        self._view.set_plot_y_range(range)

    def set_errors(self,state):
         previous = self._view.get_errors()
         print("change", state,previous)
         if previous == state:
            print("do nothing")
            return 
         self._view.set_errors(state)
