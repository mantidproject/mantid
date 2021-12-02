# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.selection_info_view import SelectionInfoView
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (get_group_or_pair_from_name,get_run_number_from_workspace_name)


class SelectionInfoPresenter(object):
    def __init__(self, context, parent):
        self._lines = {}
        self._view = SelectionInfoView(parent)
        self._view.hide()
        self._context = context

    def setup_slot_for_row_selection_changed(self, slot):
        self._view.selection_table.setup_slot_for_row_selection_changed(slot)

    def set_selected_rows_from_name(self, names):
        names_and_rows = self._view.selection_table.get_names_and_rows()
        rows = [ names_and_rows[name] for name in names]
        self._view.selection_table.set_selected_rows(rows)

    def update_lines(self, ws_list, indicies, to_plot=[]):
        self._lines = {}
        for name, index in zip(ws_list, indicies):
            self._lines[name]=index
        return self.handle_selected_workspaces_changed(ws_list, to_plot)

    def handle_selected_workspaces_changed(self, workspace_names, to_plot=[]):
        runs, groups_and_pairs = self.get_runs_groups_and_pairs(workspace_names)
        self._view.selection_table.set_workspaces(workspace_names, runs, groups_and_pairs)
        if to_plot:
            self.set_selected_rows_from_name(to_plot)
        if len(self._view.selection_table.get_selected_rows()) == 0:
            self._view.selection_table.set_selection_to_last_row()
        return self.get_selection()

    def get_selection(self):
        rows = self._view.selection_table.get_selected_rows()
        lines_to_plot = {}
        for row in rows:
            name = self._view.selection_table.get_workspace_names_from_row(row)
            lines_to_plot[name] = self._lines[name]
        return lines_to_plot

    def get_runs_groups_and_pairs(self, ws_names):
        group_pair =[]
        runs = []
        for name in ws_names:
            group_pair.append(get_group_or_pair_from_name(name))
            runs.append(get_run_number_from_workspace_name(name,self._context._data_context.instrument))
        return runs, group_pair


class PlotDataPanePresenter(BasePanePresenter):

    def __init__(self, view, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)
        self._data_type = ["Asymmetry", "Counts"]
        self._sort_by = ["Group/Pair", "Run"]
        # the model has a full context and the presenter does not
        self.selection_info = SelectionInfoPresenter(context=self._model.context,parent=self._view)
        self.update_view()
        self._view.set_select_plot_slot(self.selection_popup)
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.enable_plot_raw_option()
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)
        self.selection_info.setup_slot_for_row_selection_changed(self.plot_selection_changed)

    def plot_selection_changed(self):
        lines_to_plot = self.selection_info.get_selection()
        if lines_to_plot:
            self.add_list_to_plot(lines_to_plot.keys(), lines_to_plot.values(), hold=False, autoscale=True)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        if self._check_if_counts_and_pairs_selected():
            return
        # these will be the wrong type (asym vs. counts)
        lines_to_plot = self.selection_info.get_selection()
        new_lines_to_plot =  {}
        # this needs to be custom to handle the name conversion
        if  self._view.get_plot_type() == "Counts":
            new_lines_to_plot = {self._model.convert_ws_name_to_counts(key): value for key, value in lines_to_plot.items()}
        else:
            new_lines_to_plot = {self._model.convert_ws_name_to_asymmetry(key): value for key, value in lines_to_plot.items()}

        workspace_list, indicies = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                      self._view.get_plot_type())
        lines_to_plot = self.selection_info.update_lines(workspace_list, indicies)
        self.selection_info.set_selected_rows_from_name(new_lines_to_plot.keys())
        self.plot_lines(new_lines_to_plot, autoscale=True, hold_on=False)
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

    # want to handle extending the loaded data without unselecting stuff
    def handle_data_updated(self, autoscale=True, hold_on=False):
        previous_lines_to_plot = self.selection_info.get_selection()

        workspace_list, indicies = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                      self._view.get_plot_type())
        to_plot = [name for name in workspace_list if name in previous_lines_to_plot]
        lines_to_plot = self.selection_info.update_lines(workspace_list, indicies, to_plot)
        self.plot_lines(lines_to_plot, autoscale, hold_on)

    def plot_lines(self, lines_to_plot, autoscale, hold_on):
        if lines_to_plot:
            self.add_list_to_plot(lines_to_plot.keys(), lines_to_plot.values(), hold=hold_on, autoscale=autoscale)

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

    def selection_popup(self):
        self.selection_info._view.show()
        self.selection_info._view.raise_()
