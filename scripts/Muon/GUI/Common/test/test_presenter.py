from __future__ import (absolute_import, division, print_function)


class TestPresenter(object):

    def __init__(self,view,model):
        self.view=view
        self.model=model

    @property
    def widget(self):
        return self.view
