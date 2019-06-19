# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import construct_empty_group, construct_empty_pair
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


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
            workspace = self._groups_and_pairs[group_name].workspace[str(run)].workspace
        except AttributeError:
            workspace = self._context.calculate_group(group_name, str(run), rebin=False)
            self._groups_and_pairs[group_name].update_counts_workspace(workspace, str(run))
        return workspace

    @property
    def groups(self):
        return self._groups_and_pairs.groups

    @property
    def pairs(self):
        return self._groups_and_pairs.pairs

    @property
    def group_names(self):
        return self._groups_and_pairs.group_names

    @property
    def pair_names(self):
        return self._groups_and_pairs.pair_names

    @property
    def group_and_pair_names(self):
        return self._groups_and_pairs.group_names + self._groups_and_pairs.pair_names

    def show_all_groups_and_pairs(self):
        self._context.show_all_groups()
        self._context.show_all_pairs()

    def clear_groups(self):
        self._groups_and_pairs.clear_groups()

    def clear_pairs(self):
        self._groups_and_pairs.clear_pairs()

    def clear(self):
        self.clear_groups()
        self.clear_pairs()

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        self._groups_and_pairs.add_group(group)

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._groups_and_pairs.add_pair(pair)

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
        self._groups_and_pairs.reset_group_and_pairs_to_default(self._data.current_workspace, self._data.instrument,
                                                                self._data.main_field_direction)

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
            return round(max(self._data.get_loaded_data_for_run(self._data.current_runs[-1])['OutputWorkspace'][0].workspace.dataX(0)), 3)
        else:
            return 0.0

    def get_first_good_data_from_file(self):
        if self._data.current_runs:
            return self._data.get_loaded_data_for_run(self._data.current_runs[-1])["FirstGoodData"]
        else:
            return 0.0
