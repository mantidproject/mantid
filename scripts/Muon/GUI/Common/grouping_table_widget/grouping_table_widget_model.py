from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_data_context import MuonDataContext, construct_empty_group


class GroupingTableModel(object):
    """
    Model used when grouping table is not part of the grouping tab. When the
    grouping table presenter is embedded in the grouping tab, use the GroupingTabModel instead.

    The model takes an instance of the MuonContext as an input, most of its functionality is simply
    a callback to this class.
    """

    def __init__(self, data=MuonDataContext()):
        self._data = data

    @property
    def groups(self):
        return self._data.groups

    @property
    def group_names(self):
        return self._data.group_names

    @property
    def group_and_pair_names(self):
        return self._data.group_names + self._data.pair_names

    def add_group(self, group):
        self._data.add_group(group)

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            del self._data.groups[name]

    def construct_empty_group(self, group_index):
        return construct_empty_group(self.group_names, group_index)

    def clear_groups(self):
        self._data.clear_groups()
