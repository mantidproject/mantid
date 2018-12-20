from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid
from Muon.GUI.Common.muon_data_context import MuonDataContext


class HomeTabModel(object):

    def __init__(self, muon_data=MuonDataContext()):
        self._data = muon_data

    def is_data_loaded(self):
        return self._data.is_data_loaded()

    def loaded_instrument(self):
        return self._data.instrument

    def show_all_data(self):
        self._data.show_raw_data()
        self._data.show_all_groups()
        self._data.show_all_pairs()

    def update_current_data(self):
        self._data.update_current_data()
