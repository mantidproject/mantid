from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_data_context import MuonDataContext, construct_empty_pair


class PairingTableModel(object):

    def __init__(self, data=MuonDataContext()):
        self._data = data

    @property
    def pairs(self):
        return list(self._data.pairs)

    @property
    def pair_names(self):
        return list(self._data.pair_names)

    @property
    def group_names(self):
        return list(self._data.group_names)

    @property
    def group_and_pair_names(self):
        return list(self._data.group_names) + list(self._data.pair_names)

    def add_pair(self, pair):
        self._data.add_pair(pair)

    def remove_pairs_by_name(self, name_list):
        for name in name_list:
            del self._data.pairs[name]

    def construct_empty_pair(self, pair_index):
        return construct_empty_pair(self.group_names, self.pair_names, pair_index)

    def clear_pairs(self):
        self._data.clear_pairs()
