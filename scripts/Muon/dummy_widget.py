from __future__ import (absolute_import, division, print_function)


from Muon import dummy_view
from Muon import dummy_presenter


class DummyWidget(object):
    """
    """
    def __init__(self,name,parent=None):
        view=dummy_view.DummyView(name,parent)
        model=None
        self.presenter = dummy_presenter.DummyPresenter(view,model)

    @property
    def presneter(self):
        return self.presenter

    @property
    def widget(self):
        return self.presenter.widget

    def setButtonConnection(self,slot):
        view=self.presenter.widget
        view.buttonSignal.connect(slot)
