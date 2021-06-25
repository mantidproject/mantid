# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing


class PlotDataPanePresenter(BasePanePresenter):

    def __init__(self, view, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)
        self._data_type = ["Asymmetry", "Counts"]
        self._sort_by = ["Group/Pair", "Run"]
        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        if self._check_if_counts_and_pairs_selected():
            return
        self.handle_data_updated(autoscale=True, hold_on=False)
        # the data change probably means its the wrong scale
        self._figure_presenter.force_autoscale()

    def _check_if_counts_and_pairs_selected(self):
        if len(self.context.group_pair_context.selected_pairs) != 0 and \
                self._view.get_plot_type() == "Counts":
            self._view.set_plot_type("Asymmetry")
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')
            return True
        return False

    def handle_data_updated(self, autoscale=True, hold_on=False):
        workspace_list, indicies = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                      self._view.get_plot_type())
        self.add_list_to_plot(workspace_list, indicies, hold=hold_on, autoscale=autoscale)

    def handle_added_or_removed_group_or_pair_to_plot(self, group_pair_info):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_pair_info: A dictionary containing information on the removed group/pair
        """
        is_added = group_pair_info["is_added"]
        name = group_pair_info["name"]
        if is_added:
            self.handle_added_group_or_pair_to_plot()
        else:
            self.handle_removed_group_or_pair_from_plot(name)

    def handle_added_group_or_pair_to_plot(self):
        """
        Handles a group or pair being added from the view
        """
        unsafe_to_add = self._check_if_counts_and_pairs_selected()
        if unsafe_to_add:
            return
        self.handle_data_updated()

    def handle_removed_group_or_pair_from_plot(self, group_or_pair_name: str):
        """
        Handles a group or pair being removed in grouping widget analysis table
        :param group_or_pair_name: The group or pair name that was removed from the analysis
        """
        workspace_list = self._model.get_workspaces_to_remove(
            [group_or_pair_name], is_raw=self._view.is_raw_plot(), plot_type=self._view.get_plot_type())
        self.remove_list_from_plot(workspace_list)
        self.handle_data_updated()

    def handle_use_raw_workspaces_changed(self):
        if self.check_if_can_use_rebin():
            self.handle_data_updated()
