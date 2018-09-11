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
        context.printContext()
        self._presenter = TestPresenter(view,model)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def setUpdateContext(self,slot):
        view=self._presenter.widget
        view.buttonSignal.connect(slot)

    def updateContext(self):
       self._presenter.updateContext()

    def loadFromContext(self,context):
        model = self._presenter.model
        sub_context = model.getSubContext()
        view = self._presenter.widget
        view.loadFromContext(sub_context)    


