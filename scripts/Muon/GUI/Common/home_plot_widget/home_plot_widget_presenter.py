# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget


class HomePlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_plot_button_clicked(self.handle_plot_button_clicked)

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        pass

    def handle_plot_button_clicked(self):
        self._view.warning_popup("Plotting not currently implemented!")
