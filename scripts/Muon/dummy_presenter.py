from __future__ import (absolute_import, division, print_function)


class DummyPresenter(object):
    """
    """
    def __init__(self,view,model):
        self.view=view
        self.model=model

    def getWidget(self):
        return self.view
