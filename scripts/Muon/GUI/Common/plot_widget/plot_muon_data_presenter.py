# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, List
from Muon.GUI.Common.plot_widget.plot_widget_view_interface import PlotWidgetViewInterface
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing#, GenericObservable
from mantid.dataobjects import Workspace2D


class PlotMuonPresenter():

    def __init__(self, view: PlotWidgetViewInterface, model, context, figure_presenter):

        self._name = "Plot"
        self._data_type = [""]
        self._sort_by = [""]
        self._view = view
        self._model = model
        self.context = context
        self._figure_presenter = figure_presenter
        self._x_data_range = context.default_data_plot_range
        self.data_plot_tiled_state = False
        self._tiled_plot_state = False
        self._external_plotting_view = ExternalPlottingView()
        self._external_plotting_model = ExternalPlottingModel()
        self.add_or_remove_plot_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_plot)
        self.rebin_options_set_observer = GenericObserver(self.handle_rebin_options_changed)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self._view.on_external_plot_pressed(self.handle_external_plot_requested)
        self._view.on_plot_type_changed(self.handle_data_type_changed)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        return

    def handle_external_plot_requested(self):
        """
        Handles an external plot request
        """
        axes = self._figure_presenter.get_plot_axes()
        external_fig = self._external_plotting_view.create_external_plot_window(axes)
        data = self._external_plotting_model.get_plotted_workspaces_and_indices_from_axes(axes)
        self._external_plotting_view.plot_data(external_fig, data)
        self._external_plotting_view.copy_axes_setup(external_fig, axes)
        self._external_plotting_view.show(external_fig)

    def handle_workspace_replaced_in_ads(self, workspace: Workspace2D):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in the ADS
        and updates to the values from recalculation.
        :param workspace: workspace 2D object
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.replace_workspace_in_plot(workspace)

    def handle_added_or_removed_plot(self, plot_info: Dict):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_pair_info: A dictionary containing information on the removed group/pair
        """
        is_added = plot_info["is_added"]
        name = plot_info["name"]
        if is_added:
            self.add_to_plot(name)
        else:
            self.remove_from_plot(name)

    def add_list_to_plot(self, ws_names: List[str], indicies: List[int],hold: bool = False, autoscale=False):

        self._figure_presenter.plot_workspaces(ws_names, indicies, hold_on=hold, autoscale=False)

    def add_to_plot(self, ws_name: str, hold: bool = False):

        self._figure_presenter.plot_workspaces([ws_name], [0], hold_on=hold, autoscale=False)

    def remove_list_from_plot(self, names: List[str]):
        self._figure_presenter.remove_workspace_names_from_plot(names)

    def update_view(self):
        self._view.setup_plot_type_options(self._data_type)
        self._view.setup_tiled_by_options(self._sort_by)

    @property
    def view(self):
        return self._view

    @property
    def name(self) -> str:
        return self._name

    @property
    def data_types(self)->List[str]:
        return self._data_types

    @property
    def sort_by(self)->List[str]:
        return self._sort_by

    def plot_mode_changed(self):
        return

    def update_range(self, range:List[float]):
        self._x_data_range = range

    @property
    def get_range(self)->List[float]:
        return self._x_data_range

    def update_tiled_state(self, state):
        self._tiled_plot_state = state

    @property
    def tiled_state(self):
        return self.tiled_plot_state

    def hide(self):
        self._view.hide()

    def show(self):
        self._view.show()

    def handle_rebin_options_changed(self):
        """
        Handles rebin options changed on the home tab
        """
        if self.context._do_rebin():
            self._view.set_raw_checkbox_state(False)
        else:
            self._view.set_raw_checkbox_state(True)

    def handle_use_raw_workspaces_changed(self):
        return


class PlotMuonDataPresenter(PlotMuonPresenter):

    def __init__(self, view: PlotWidgetViewInterface, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)
        self._name = "Plot Data"
        self._data_type = ["Asymmetry", "Counts"]
        self._sort_by = ["Group/Pair", "Run"]
        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()
        self.data_changed_observer = GenericObserver(self.handle_data_updated)
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        if self._check_if_counts_and_groups_selected():
            return

        self.handle_data_updated(autoscale=True, hold_on=False)
        #self.plot_type_changed_notifier.notify_subscribers(self._view.get_plot_type())

    def _check_if_counts_and_groups_selected(self):
        if len(self.context.group_pair_context.selected_pairs) != 0 and \
                self._view.get_plot_type() == "Counts":
            self._view.set_plot_type("Asymmetry")
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')
            return True
        return False

    def plot_mode_changed(self):
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()
        self._view.set_is_tiled_plot(self.data_plot_tiled_state)

    def handle_data_updated(self, autoscale=True, hold_on=False):
        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                     self._view.get_plot_type())
        self.add_list_to_plot(workspace_list, indices, hold_on, autoscale)

    def handle_added_or_removed_group_or_pair_to_plot(self, group_pair_info: Dict):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_pair_info: A dictionary containing information on the removed group/pair
        """
        is_added = group_pair_info["is_added"]
        name = group_pair_info["name"]
        if is_added:
            self.handle_added_group_or_pair_to_plot(name)
        else:
            self.handle_removed_group_or_pair_to_plot(name)

    def handle_added_group_or_pair_to_plot(self, group_or_pair_name: str):
        """
        Handles a group or pair being added from the view
        :param group_or_pair_name: The group or pair name that was added to the analysis
        """
        unsafe_to_add = self._check_if_counts_and_groups_selected()
        if unsafe_to_add:
            return

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair(
            group_or_pair_name, is_raw=self._view.is_raw_plot(), plot_type=self._view.get_plot_type())
        self.add_list_to_plot(workspace_list, indices, hold=True)

    def handle_removed_group_or_pair_to_plot(self, group_or_pair_name: str):
        """
        Handles a group or pair being removed in grouping widget analysis table
        :param group_or_pair_name: The group or pair name that was removed from the analysis
        """
        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair(
            group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self.remove_list_from_plot(workspace_list)

    def handle_use_raw_workspaces_changed(self):
        self.handle_data_updated()


class PlotMuonFreqPresenter(PlotMuonPresenter):

    def __init__(self, view: PlotWidgetViewInterface, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)

        self._name = "Plot Freq"
        self._data_type = ["Frequency"]
        self._sort_by = ["spcetra", "Run"]

        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()

    def plot_mode_changed(self):
        self._view.disable_plot_type_combo()
        self._view.show_plot_diff()
        self.data_plot_tiled_state = self._view.is_tiled_plot()
