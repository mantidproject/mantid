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

    def connect_errors_changed(self,slot):
        self._presenter.connect_errors_changed(slot)

    def connect_x_range_changed(self,slot):
        self._presenter.connect_x_range_changed(slot)

    def connect_y_range_changed(self,slot):
        self._presenter.connect_y_range_changed(slot)

    def loadFromContext(self):
        model = self._presenter.model
        sub_context = model.getSubContext()
        # update the view with the subcontext
        view = self._presenter.widget
        view.loadFromContext(sub_context)