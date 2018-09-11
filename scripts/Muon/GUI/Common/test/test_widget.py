from __future__ import (absolute_import, division, print_function)


from Muon.GUI.Common.test.test_view import TestView
from Muon.GUI.Common.test.test_presenter import TestPresenter
from Muon.GUI.Common.test.test_model import TestModel


class TestWidget(object):
    """
	An example of how to use the context with a widget class. 
	The widget class exposes the MVP to the rest of the GUI
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

    # interact with context
    def setUpdateContext(self,slot):
        """ 
        This function is to set the update
        method from the main GUI to the signals
        from this GUI
        """
        view=self._presenter.widget
        view.updateSignal.connect(slot)

    def updateContext(self):
       self._presenter.updateContext()

    def loadFromContext(self,context):
        # extract relevant info from context via model
        model = self._presenter.model
        sub_context = model.getSubContext()
        # update the view with the subcontext
        view = self._presenter.widget
        view.loadFromContext(sub_context)    


