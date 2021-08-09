# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing


class DuelPlotMaxentPanePresenter(BasePanePresenter):

    def __init__(self, view, model, context, figure_presenter):
        super().__init__(view, model, context, figure_presenter)
        self._data_type = ["Maxent and Counts"]
        self._sort_by = ["Maxent + Groups/detectors"]
        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.hide_plot_raw()
        self._view.disable_tile_plotting_options()
        self._view.set_is_tiled_plot(True)
        #self._view.enable_plot_raw_option()
        self.method_changed =  GenericObserverWithArgPassing(self.change_time_plot)
        self.new_data_observer = GenericObserverWithArgPassing(
            self.handle_maxent_data_updated)
        self.reconstructed_data_observer = GenericObserverWithArgPassing(
            self.handle_reconstructed_data_updated)
        self._time_data = "groups"

    def change_time_plot(self, if_groups):
        self._time_data ="groups" if if_groups else "all"
        self.handle_time_data_updated()

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        self.handle_time_data_updated()
        # the data change probably means its the wrong scale
        self._figure_presenter.force_autoscale()

    def handle_time_data_updated(self):
        return self._model.get_workspace_list_and_indices_to_plot(self._time_data=="groups")

    def handle_maxent_data_updated(self, name):
        self._maxent_ws_name = name
        self.add_data_to_plots()

    def add_data_to_plots(self):
        workspaces, indicies = self.handle_time_data_updated()
        workspaces, indicies = self._model.add_reconstructed_data(workspaces, indicies)
        workspaces += [self._maxent_ws_name]
        indicies += [0]
        self.add_list_to_plot(workspaces, indicies, hold=False, autoscale=True)

    def handle_reconstructed_data_updated(self, data_dict):
        self._model.clear_reconstructed_data()
        if data_dict["table"] and data_dict["ws"]:
            self._model.set_reconstructed_data(data_dict["ws"], data_dict["table"])
        self.add_data_to_plots()
