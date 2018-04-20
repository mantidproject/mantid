from __future__ import (absolute_import, division, print_function)


import mantid.simpleapi as mantid
from Muon import dummy_view
from Muon import dummy_presenter


class DummyWidget(object):
    """
    """
    def __init__(self,name,parent=None):
        view=dummy_view.DummyView(name,parent)
        model=None
        self.presenter = dummy_presenter.DummyPresenter(view,model)

    def getPresneter(self):
        return self.presenter

    def getWidget(self):
        return self.presenter.getWidget()

    def setButtonConnection(self,slot):
        view=self.presenter.getWidget()
        view.buttonSignal.connect(slot)
