from __future__ import (absolute_import, division, print_function)


class TestPresenter(object):

    def __init__(self,view,model):
        self.model=model
        self.view=view

    @property
    def widget(self):
        return self.view

    def loadFromContext(self):
        return self.model.getSubContext()
