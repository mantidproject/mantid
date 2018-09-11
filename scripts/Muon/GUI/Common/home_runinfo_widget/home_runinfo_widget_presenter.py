from __future__ import (absolute_import, division, print_function)


class HomeRunInfoWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

    def show(self):
        self._view.show()