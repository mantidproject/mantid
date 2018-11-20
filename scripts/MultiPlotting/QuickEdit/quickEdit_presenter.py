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

    def connect_errors_changed(self,state):
        self._view.connect_errors_changed(state)

    def autoscale_changed(self,state):
        self._view.change_autoscale(state)

    def connect_x_range_changed(self,slot):
        self._view.connect_x_range_changed(slot)

    def connect_y_range_changed(self,slot):
        self._view.connect_y_range_changed(slot)

    def updateContext(self,range):
        subContext = self._view.getSubContext()
        self.model.updateContext(subContext)
