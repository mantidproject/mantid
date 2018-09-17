from __future__ import (absolute_import, division, print_function)


class ContextExamplePresenter(object):

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
        # the groups are all row 0
        if row == 0:
            # make sure the context is up to date
            self.updateContext()
            # update the whole GUI (not just this MVP)
            self._view.sendUpdateSignal()

    def updateContext(self):
      """
      A simple method for updating the context.
      Gets the current values from the view and the model
      will update the context accordingly (inc. packing the data)
      """
      subcontext = self._view.getSubContext()
      self._model.updateContext(subcontext)
