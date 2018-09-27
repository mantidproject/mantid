from __future__ import (absolute_import, division, print_function)

from collections import OrderedDict

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_context import MuonContext


class GroupingTabModel(object):
    """
    The model for the grouping tab should be shared between all widgets of the tab.
    It keeps a record of the groups and pairs defined for the current instance of the interface.

    pairs and groups should be of type MuonGroup and MuonPair respectively.
    """

    def __init__(self, data=MuonContext()):
        self._data = data

    def get_group_workspace(self, group_name):
        """
        Return the workspace associated to group_name, creating one if
        it doesn't already exist (e.g. if group added to table but no update yet triggered).
        """
        try:
            workspace = self._data.groups[group_name].workspace.workspace
        except AttributeError:
            self._data.show_group_data(group_name, show=False)
            workspace = self._data.groups[group_name].workspace.workspace
        return workspace

    @property
    def groups(self):
        return self._data.groups.values()

    @property
    def pairs(self):
        return self._data.pairs.values()

    @property
    def group_names(self):
        return self._data.groups.keys()

    @property
    def pair_names(self):
        return self._data.pairs.keys()

    @property
    def group_and_pair_names(self):
        return self.group_names + self.pair_names

    def show_all_groups_and_pairs(self):
        self._data.show_all_groups()
        self._data.show_all_pairs()

    def clear_groups(self):
        self._data._groups = OrderedDict()

    def clear_pairs(self):
        self._data._pairs = OrderedDict()

    def clear(self):
        self.clear_groups()
        self.clear_pairs()

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        self._data.groups[group.name] = group

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._data.pairs[pair.name] = pair

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            del self._data.groups[name]

    def remove_pairs_by_name(self, name_list):
        for name in name_list:
            del self._data.pairs[name]

    def construct_empty_group(self, _group_index):
        return self._data.construct_empty_group(_group_index)

    def construct_empty_pair(self, _pair_index):
        return self._data.construct_empty_pair(_pair_index)

    def construct_empty_pair_with_group_names(self, name1, name2):
        """
        Create a default pair with specific group names.
        The pair name is auto-generated and alpha=1.0
        """
        pair = self._data.construct_empty_pair(0)
        pair.group1 = name1
        pair.group2 = name2
        return pair

    def reset_groups_and_pairs_to_default(self):
        self._data.set_groups_and_pairs_to_default()

    def update_pair_alpha(self, pair_name, new_alpha):
        self._data.pairs[pair_name].alpha = new_alpha

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
