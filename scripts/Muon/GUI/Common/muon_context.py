# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, estimate_group_asymmetry_data


class MuonContext(object):
    def __init__(self, muon_data_context=MuonDataContext(), muon_gui_context=MuonGuiContext(), muon_group_context=MuonGroupPairContext()):
        self._data_context = muon_data_context
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context

    @property
    def data_context(self):
        return self._data_context

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def group_pair_context(self):
        return self._group_pair_context

    def calculate_group(self, group_name, run, rebin=False):
        group_workspace = calculate_group_data(self, group_name, run, rebin)
        group_asymmetry = estimate_group_asymmetry_data(self, group_name, run, rebin)

        self._group_pair_context[group_name].update_workspace(group_workspace, group_asymmetry)

