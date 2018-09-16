from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_context import MuonContext

from collections import OrderedDict

class GroupingTableModel(object):

    def __init__(self,data = MuonContext()):

        self._data = data
        #self._pairs = {}

    @property
    def pairs(self):
        return self._data._pairs

    @property
    def pair_names(self):
        return self._data._pairs.keys()

    def add_pair(self, pair):
        self._data._pairs[pair.name] = pair

    def construct_empty_pair(self, pair_index):
        return MuonPair(pair_name="pair_" + str(pair_index))

    def remove_pairs_by_name(self, name_list):
        for name in name_list:
            del self._data._pairs[name]

    def clear(self):
        self._data._pairs = OrderedDict()
