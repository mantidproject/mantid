# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.observer_pattern import GenericObserver

class HomePlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._plot_window = None
        self._view.on_plot_button_clicked(self.handle_new_data_loaded)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self.input_workspace_observer = GenericObserver(self.handle_new_data_loaded)
        self.fit_observer = GenericObserver(self.handle_fit_completed)
        self.keep = False
        self.plotted_workspaces = []

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        pass

    def handle_new_data_loaded(self):
        if self.plotted_workspaces == self._model.get_workspaces_to_plot(self._view.if_raw()):
            # If the workspace names have not changed the ADS observer should handle any updates
            return

        self.plot_standard_workspaces()

    def handle_use_raw_workspaces_changed(self):
        if not self._view.if_raw() and not self._model.context._do_rebin():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup('No rebin options specified')
            return

        self.plot_standard_workspaces()

    def plot_standard_workspaces(self):
        if self._plot_window:
            self._plot_window.window.close()
        self.plotted_workspaces = self._model.get_workspaces_to_plot(self._view.if_raw())
        self._plot_window = self._model.create_new_plot(self.plotted_workspaces,
                                                        self._model.get_plot_title(), self._close_plot)
        self._plot_window.show()

    def add_plot(self, workspace_name):
        if not self._plot_window:
            return

        self._plot_window.multi_plot.plot(self._model.get_plot_title(), workspace_name, specNum=3)

    def handle_fit_completed(self):
        for plotted_workspace in self.plotted_workspaces:
            list_of_fits = self._model.context.fitting_context.find_fit_for_input_workspace_name(plotted_workspace)
            list_of_workspaces_to_plot = [fit.parameter_name.replace(' Parameters', '') for fit in list_of_fits]
            for workspace_name in list_of_workspaces_to_plot:
                self.add_plot(workspace_name)

    def _close_plot(self):
        self._plot_window = None
        self.plotted_workspaces = []
