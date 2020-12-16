# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_data_context import construct_empty_group
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.Common.muon_group import MuonRun
from enum import Enum


class RowValid(Enum):
    invalid_for_all_runs = 0
    valid_for_all_runs = 2
    valid_for_some_runs = 1


class EAGroupingTabModel(object):
    """
    The model for the grouping tab should be shared between all widgets of the tab.
    It keeps a record of the groups and pairs defined for the current instance of the interface.

    pairs and groups should be of type MuonGroup and MuonPair respectively.
    """

    def __init__(self, context=None):
        self._context = context
        self._data = context.data_context
        self._groups = context.group_context
        self._gui_variables = context.gui_context

    def get_group_workspace(self, group_name, run):
        """
        Return the workspace associated to group_name, creating one if
        it doesn't already exist (e.g. if group added to table but no update yet triggered).
        """
        try:
            workspace = self._groups[group_name].workspace[MuonRun(run)].workspace
        except AttributeError:
            workspace = self._context.calculate_group(group_name, run, rebin=False)
            self._groups[group_name].update_counts_workspace(workspace, MuonRun(run))
        return workspace

    @property
    def groups(self):
        return self._groups.groups

    @property
    def group_names(self):
        return self._groups.group_names

    @property
    def selected_groups(self):
        return self._groups.selected_groups

    def show_all_groups(self):
        self._context.show_all_groups()

    def clear_groups(self):
        self._groups.clear_groups()

    def clear_selected_groups(self):
        self._groups.clear_selected_groups()

    def clear(self):
        self.clear_groups()
        self.clear_selected_groups()

    def select_all_groups_to_analyse(self):
        self._groups.set_selected_groups_to_all()

    def remove_group_from_analysis(self, group):
        self._groups.remove_group_from_selected_groups(group)

    def add_group_to_analysis(self, group):
        self._groups.add_group_to_selected_groups(group)

    def add_group(self, group):
        assert isinstance(group, EAGroup)
        self._groups.add_group(group)

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            self._groups.remove_group(name)

    def construct_empty_group(self, _group_index):
        return construct_empty_group(self.group_names, _group_index)

    def reset_groups_to_default(self):
        if not self._context.current_runs:
            return "failed"

        self._groups.reset_group_to_default(self._data.current_workspace, self._data.instrument)
        return "success"

    def reset_selected_groups(self):
        self._groups.reset_selected_groups()

    @property
    def num_detectors(self):
        return self._data.num_detectors

    @property
    def instrument(self):
        return self._data.instrument

    @property
    def main_field_direction(self):
        return self._data.main_field_direction

    def is_data_loaded(self):
        return self._data.is_data_loaded()

    def get_last_data_from_file(self):
        if self._data.current_runs:
            return round(max(
                self._data.get_loaded_data_for_run(self._data.current_runs[-1])['OutputWorkspace'][0].workspace.dataX(
                    0)), 3)
        else:
            return 0.0

    def get_first_good_data_from_file(self):
        if self._data.current_runs:
            return self._data.get_loaded_data_for_run(self._data.current_runs[-1])["FirstGoodData"]
        else:
            return 0.0
