from __future__ import (absolute_import, division, print_function)

from collections import OrderedDict

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_context import MuonContext

class GroupingTableModel(object):
    """
    Model used when grouping table is not part of the grouping tab. When the
    grouping table presenter is embedded in the grouping tab, use the GroupingTabModel instead.

    """

    def __init__(self, data = MuonContext()):

        self._data = data
        #self._groups = {}

    @property
    def groups(self):
        return self._data._groups.values()

    @property
    def group_names(self):
        return self._data._groups.keys()

    def add_group(self, group):
        self._data._groups[group.name] = group

    def construct_empty_group(self, group_index):
        return MuonGroup(group_name="group_" + str(group_index), detector_IDs=[1])

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            del self._data._groups[name]

    def clear(self):
        self._data._groups = OrderedDict()
