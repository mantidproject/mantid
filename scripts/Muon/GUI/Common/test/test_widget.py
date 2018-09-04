from __future__ import (absolute_import, division, print_function)


from Muon.GUI.Common.test.test_view import TestView
from Muon.GUI.Common.test.test_presenter import TestPresenter


class TestWidget(object):
    """
    """
    def __init__(self,parent=None):
        view=TestView(parent)
        model=None
        self._presenter = TestPresenter(view,model)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def setButtonConnection(self,slot):
        view=self._presenter.widget
        view.buttonSignal.connect(slot)
