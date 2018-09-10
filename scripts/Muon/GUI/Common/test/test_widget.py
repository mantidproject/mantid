from __future__ import (absolute_import, division, print_function)


from Muon.GUI.Common.test.test_view import TestView
from Muon.GUI.Common.test.test_presenter import TestPresenter
from Muon.GUI.Common.test.test_model import TestModel


class TestWidget(object):
    """
    """
    def __init__(self,context,parent=None):
        model=TestModel(context)
        sub_context = model.getSubContext()
        view=TestView(sub_context,parent)
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
