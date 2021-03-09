# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_data_context import construct_empty_group, construct_empty_pair
from Muon.GUI.Common.muon_group import MuonGroup, MuonDiff
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_group import MuonRun
from enum import Enum


class RowValid(Enum):
    invalid_for_all_runs = 0
    valid_for_all_runs = 2
    valid_for_some_runs = 1


class GroupingTabModel(object):
    """
    The model for the grouping tab should be shared between all widgets of the tab.
    It keeps a record of the groups and pairs defined for the current instance of the interface.

    pairs and groups should be of type MuonGroup and MuonPair respectively.
    """

    def __init__(self, context=None):
        self._context = context
        self._data = context.data_context
        self._groups_and_pairs = context.group_pair_context
        self._gui_variables = context.gui_context

    def get_group_workspace(self, group_name, run):
        """
        Return the workspace associated to group_name, creating one if
        it doesn't already exist (e.g. if group added to table but no update yet triggered).
        """
        try:
            workspace = self._groups_and_pairs[group_name].workspace[MuonRun(run)].workspace
        except AttributeError:
            workspace = self._context.calculate_group(group_name, run, rebin=False)
            self._groups_and_pairs[group_name].update_counts_workspace(workspace, MuonRun(run))
        return workspace

    @property
    def groups(self):
        return self._groups_and_pairs.groups

    @property
    def pairs(self):
        return self._groups_and_pairs.pairs

    @property
    def diffs(self):
        return self._groups_and_pairs.diffs

    @property
    def group_names(self):
        return self._groups_and_pairs.group_names

    @property
    def diff_names(self):
        return self._groups_and_pairs.diff_names

    @property
    def pair_names(self):
        return self._groups_and_pairs.pair_names

    def get_diffs(self, group_or_pair):
        return self._groups_and_pairs.get_diffs(group_or_pair)

    def get_names(self, group_or_pair):
        if group_or_pair == "group":
            return self.group_names
        elif group_or_pair == "pair":
            return self.pair_names
        else:
            return []

    @property
    def group_and_pair_names(self):
        return self._groups_and_pairs.group_names + self._groups_and_pairs.pair_names + self._groups_and_pairs.diff_names

    @property
    def selected_groups(self):
        return self._groups_and_pairs.selected_groups

    @property
    def selected_pairs(self):
        return self._groups_and_pairs.selected_pairs

    @property
    def selected_diffs(self):
        return self._groups_and_pairs.selected_diffs

    @property
    def selected_groups_and_pairs(self):
        return self.selected_groups+self.selected_pairs+self.selected_diffs

    def show_all_groups_and_pairs(self):
        self._context.show_all_groups()
        self._context.show_all_pairs()
        self._context.show_all_diffs()

    def clear_groups(self):
        self._groups_and_pairs.clear_groups()

    def clear_pairs(self):
        self._groups_and_pairs.clear_pairs()

    def clear_diffs(self, group_or_pair):
        self._groups_and_pairs.clear_diffs(group_or_pair)

    def clear_selected_pairs(self):
        self._groups_and_pairs.clear_selected_pairs()

    def clear_selected_groups(self):
        self._groups_and_pairs.clear_selected_groups()

    def clear_selected_diffs(self):
        self._groups_and_pairs.clear_selected_diffs()

    def clear(self):
        self.clear_groups()
        self.clear_pairs()
        self.clear_diffs("group")
        self.clear_diffs("pair")
        self.clear_selected_groups()
        self.clear_selected_diffs()
        self.clear_selected_pairs()

    def select_all_groups_to_analyse(self):
        self._groups_and_pairs.set_selected_groups_to_all()

    def check_group_in_use(self, name):
        used_by = ""
        # check pairs
        for pair in self._groups_and_pairs.pairs:
            if name == pair.forward_group or name == pair.backward_group:
                used_by += pair.name + ", "
        for diff in self._groups_and_pairs.diffs:
            if name == diff.forward_group or name == diff.backward_group:
                used_by += diff.name +", "
        if used_by:
            # the -2 removes the space and comma
            return name + " is used by: "+ used_by[0:-2]
        else:
            return used_by

    def check_pair_in_use(self, name):
        used_by = ""
        # check diffs
        for diff in self._groups_and_pairs.diffs:
            if name == diff.forward_group or name == diff.backward_group:
                used_by += diff.name +", "
        if used_by:
            # the -2 removes the space and comma
            return name + " is used by: "+ used_by[0:-2]
        else:
            return used_by

    def remove_group_from_analysis(self, group):
        self._groups_and_pairs.remove_group_from_selected_groups(group)

    def add_group_to_analysis(self, group):
        self._groups_and_pairs.add_group_to_selected_groups(group)

    def remove_pair_from_analysis(self, pair):
        self._groups_and_pairs.remove_pair_from_selected_pairs(pair)

    def add_pair_to_analysis(self, pair):
        self._groups_and_pairs.add_pair_to_selected_pairs(pair)

    def remove_diff_from_analysis(self, diff):
        self._groups_and_pairs.remove_diff_from_selected_diffs(diff)

    def add_diff_to_analysis(self, diff):
        self._groups_and_pairs.add_diff_to_selected_diffs(diff)

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        self._groups_and_pairs.add_group(group)

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._groups_and_pairs.add_pair(pair)

    def add_diff(self, diff):
        assert isinstance(diff, MuonDiff)
        self._groups_and_pairs.add_diff(diff)

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            self._groups_and_pairs.remove_group(name)
            self.remove_pairs_with_removed_name(name)

    def remove_pairs_with_removed_name(self, group_name):
        for pair in self._groups_and_pairs.pairs:
            if pair.forward_group == group_name or pair.backward_group == group_name:
                self._groups_and_pairs.remove_pair(pair.name)

    def remove_pairs_by_name(self, name_list):
        for name in name_list:
            self._groups_and_pairs.remove_pair(name)

    def remove_diffs_by_name(self, name_list):
        for name in name_list:
            self._groups_and_pairs.remove_diff(name)

    def construct_empty_group(self, _group_index):
        return construct_empty_group(self.group_names, _group_index)

    def construct_empty_pair(self, _pair_index):
        return construct_empty_pair(self.group_names, self.pair_names, _pair_index)

    def construct_empty_pair_with_group_names(self, name1, name2):
        """
        Create a default pair with specific group names.
        The pair name is auto-generated and alpha=1.0
        """
        pair = construct_empty_pair(self.group_names, self.pair_names, 0)
        pair.forward_group = name1
        pair.backward_group = name2
        return pair

    def reset_groups_and_pairs_to_default(self):
        if not self._context.current_runs:
            return "failed"
        maximum_number_of_periods = max([self._context.num_periods(run) for run in self._context.current_runs])

        self._groups_and_pairs.reset_group_and_pairs_to_default(self._data.current_workspace, self._data.instrument,
                                                                self._data.main_field_direction, maximum_number_of_periods)
        return "success"

    def reset_selected_groups_and_pairs(self):
        self._groups_and_pairs.reset_selected_groups_and_pairs()

    def update_pair_alpha(self, pair_name, new_alpha):
        self._groups_and_pairs[pair_name].alpha = new_alpha

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

    # ------------------------------------------------------------------------------------------------------------------
    # Periods
    # ------------------------------------------------------------------------------------------------------------------

    def is_data_multi_period(self):
        return self._data.is_multi_period()

    def number_of_periods(self):
        if self.is_data_multi_period():
            return len(self._data.current_data["OutputWorkspace"])
        else:
            return 1

    def validate_periods_list(self, periods):
        invalid_runs = []
        current_runs = self._context.current_runs

        for run in current_runs:
            if any([period < 1 or self._context.num_periods(run) < period for period in periods]):
                invalid_runs.append(run)

        if not invalid_runs:
            return RowValid.valid_for_all_runs
        elif len(invalid_runs) == len(current_runs):
            return RowValid.invalid_for_all_runs
        else:
            return RowValid.valid_for_some_runs

    def get_periods(self, name):
        return self._groups_and_pairs[name].periods
