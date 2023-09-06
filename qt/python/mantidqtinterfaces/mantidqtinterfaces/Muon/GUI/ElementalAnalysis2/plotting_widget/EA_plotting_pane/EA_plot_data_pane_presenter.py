# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing


class EAPlotDataPanePresenter(BasePanePresenter):
    def __init__(self, view, model, context, figure_presenter):
        super(EAPlotDataPanePresenter, self).__init__(view, model, context, figure_presenter)
        self._data_type = ["Total", "Prompt", "Delayed"]
        self._sort_by = ["Detector", "Run"]
        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()
        self.data_changed_observer = GenericObserverWithArgPassing(self.handle_added_or_removed_group_to_plot)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        self.handle_data_updated(autoscale=True, hold_on=False)
        # the data change probably means its the wrong scale
        self._figure_presenter.force_autoscale()

    def handle_data_updated(self, autoscale=True, hold_on=False):
        self.check_if_can_use_rebin()
        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(), self._view.get_plot_type())
        self.add_list_to_plot(workspace_list, indices, hold=hold_on, autoscale=autoscale)

    def handle_added_or_removed_group_to_plot(self, group_info):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_info:
        """
        is_added = group_info["is_added"]
        name = group_info["name"]

        if is_added:
            self.handle_added_group_to_plot()
        else:
            self.handle_removed_group_from_plot(name)
        self._figure_presenter._handle_autoscale_y_axes()

    def handle_added_group_to_plot(self):
        """
        Handles a group being added from the view
        """
        self.handle_data_updated()

    def handle_removed_group_from_plot(self, group_name: str):
        """
        Handles a group being removed in grouping widget analysis table
        :param group_name: The group name that was removed from the analysis
        """
        workspace_list = self._model.get_workspaces_to_remove([group_name])
        self.remove_list_from_plot(workspace_list)
        self.handle_data_updated()

    def handle_use_raw_workspaces_changed(self):
        if self.check_if_can_use_rebin():
            self.handle_data_updated()

    def check_if_can_use_rebin(self):
        if not self._view.is_raw_plot() and not self.check_selected_groups_if_rebinned():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup("No rebin options specified")
            return False
        return True

    def handle_workspace_replaced_in_ads(self, workspace):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in the ADS
        and updates to the values from recalculation.
        :param workspace: workspace 2D object
        """
        if isinstance(workspace, str):
            workspace_name = workspace
        else:
            workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.replace_workspace_in_plot(workspace)
        self.handle_data_updated()

    def check_selected_groups_if_rebinned(self):
        """
        Checkes if at least one selected groups in context have a rebinned workspace
        """
        is_rebinned = []
        for group_name in self.context.group_context.selected_groups:
            group = self.context.group_context[group_name]
            is_rebinned.append(group.is_rebinned_workspace_present())
        return any(is_rebinned)

    def _update_tile_plot(self):
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            self.context.plot_panes_context[self.name].settings.set_tiled_by(tiled_by)
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys)
            self._figure_presenter._handle_autoscale_y_axes()

    def handle_workspace_deleted_from_ads(self, workspace):
        """
        Handles a workspace being deleted from ads by removing the workspace from the plot
        :param workspace: workspace 2D object
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.remove_workspace_from_plot(workspace)
        self.handle_data_updated()
