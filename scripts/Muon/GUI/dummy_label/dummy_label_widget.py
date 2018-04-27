from __future__ import (absolute_import, division, print_function)

from Muon.GUI.dummy_label.dummy_label_view import DummyLabelView
from Muon.GUI.dummy_label.dummy_label_presenter import DummyLabelPresenter


class DummyLabelWidget(object):

    def __init__(self, name, parent=None):
        view = DummyLabelView(name, parent)
        model = None
        self.presenter = DummyLabelPresenter(view, model)

    def getPresenter(self):
        return self.presenter

    @property
    def widget(self):
        return self.presenter.widget

    def updateLabel(self, message):
        self.presenter.updateLabel(message)
