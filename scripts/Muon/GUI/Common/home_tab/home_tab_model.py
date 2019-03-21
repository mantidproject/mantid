# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid
from Muon.GUI.Common.muon_context import MuonContext


class HomeTabModel(object):

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data.data_context

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
