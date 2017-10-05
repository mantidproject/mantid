from __future__ import (absolute_import, division, print_function)


class TransformSelectionPresenter(object):

    def __init__(self,view):
        self.view=view

    def setMethodsCombo(self,options):
        self.view.setMethodsCombo(options)
