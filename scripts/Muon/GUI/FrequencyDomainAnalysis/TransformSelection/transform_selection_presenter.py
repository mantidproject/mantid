from __future__ import (absolute_import, division, print_function)


class TransformSelectionPresenter(object):

    """
    The widget for selecting the widget
    shown in the transformation tab
    """

    def __init__(self, view):
        self.view = view

    @property
    def widget(self):
        return self.view

    def setMethodsCombo(self, options):
        self.view.setMethodsCombo(options)
