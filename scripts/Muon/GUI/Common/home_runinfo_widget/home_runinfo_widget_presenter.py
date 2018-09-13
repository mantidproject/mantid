from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget

class HomeRunInfoWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        pass