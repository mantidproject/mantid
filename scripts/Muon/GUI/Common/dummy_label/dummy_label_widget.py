# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.dummy_label.dummy_label_view import DummyLabelView
from Muon.GUI.Common.dummy_label.dummy_label_presenter import DummyLabelPresenter
from Muon.GUI.Common.dummy_label.dummy_label_model import DummyLabelModel


class DummyLabelWidget(object):

    def __init__(self, context, key, parent=None):
        model = DummyLabelModel(context, key)
        sub_context = model.getSubContext()
        view = DummyLabelView(sub_context, parent)
        self._presenter = DummyLabelPresenter(view, model)

    def getPresenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def updateLabel(self, message):
        self._presenter.updateLabel(message)

    # interact with context
    def updateContext(self):
        self._presenter.updateContext()

    def loadFromContext(self):
        # extract relevant info from context via model
        model = self._presenter.model
        sub_context = model.getSubContext()
        # update the view with the subcontext
        view = self._presenter.widget
        view.loadFromContext(sub_context)
