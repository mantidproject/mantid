from __future__ import (absolute_import, division, print_function)


from Muon.GUI.dummy.dummy_view import DummyView
from Muon.GUI.dummy.dummy_presenter import DummyPresenter


class DummyWidget(object):
    """
    """
    def __init__(self,name,parent=None):
        view=DummyView(name,parent)
        model=None
        self.presenter = DummyPresenter(view,model)

    @property
    def presneter(self):
        return self.presenter

    @property
    def widget(self):
        return self.presenter.widget

    def setButtonConnection(self,slot):
        view=self.presenter.widget
        view.buttonSignal.connect(slot)
