from __future__ import (absolute_import, division, print_function)


class TestPresenter(object):

    def __init__(self,view,model):
        self._model=model
        self._view=view
        self._view.groupChangedSignal.connect(self.groupChanged)

    @property
    def widget(self):
        return self._view

    @property
    def model(self):
        return self._model

    def groupChanged(self,row):
        if row == 0:
            self.updateContext()
            self._view.buttonClick()

    def updateContext(self):
      subcontext = self._view.getSubContext()
      self._model.updateContext(subcontext)

    def loadFromContext(self):
        return self._model.getSubContext()
