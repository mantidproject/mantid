from __future__ import (absolute_import, division, print_function)


from Muon import dummy_label_view
from Muon import dummy_label_presenter


class DummyLabelWidget(object):

    """
    """

    def __init__(self, name, parent=None):
        view = dummy_label_view.DummyLabelView(name, parent)
        model = None
        self.presenter = dummy_label_presenter.DummyLabelPresenter(view, model)

    def getPresneter(self):
        return self.presenter

    def getWidget(self):
        return self.presenter.getWidget()

    def updateLabel(self, message):
        self.presenter.updateLabel(message)
