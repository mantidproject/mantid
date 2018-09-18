from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_context import MuonContext


class PairingTableModel(object):

    def __init__(self, data=MuonContext()):
        self._data = data

    @property
    def pairs(self):
        return self._data.pairs

    @property
    def pair_names(self):
        return self._data.pair_names

    def add_pair(self, pair):
        self._data.add_pair(pair)

    def remove_pairs_by_name(self, name_list):
        for name in name_list:
            del self._data.pairs[name]

    def construct_empty_pair(self, pair_index):
        # return MuonPair(pair_name="pair_" + str(pair_index))
        return self._data.construct_empty_pair(pair_index)

    def clear(self):
        self._data.clear_pairs()
