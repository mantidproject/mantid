# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.utilities.algorithm_utils import run_Fit, run_simultaneous_Fit, run_CalculateMuonAsymmetry
import mantid
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantid.simpleapi import (RenameWorkspace, ConvertFitFunctionForMuonTFAsymmetry, CopyLogs, EvaluateFunction)
from mantid.api import AnalysisDataService


class SeqFittingTabModel(object):
    def __init__(self, context):
        self.context = context

    def get_selected_workspace_list(self):
        selected_workspaces = []
        selected_groups_and_pairs = self._get_selected_groups_and_pairs()
        for grp_and_pair in selected_groups_and_pairs:
            selected_workspaces += self.context.get_names_of_workspaces_to_fit(
                runs='All',
                group_and_pair=grp_and_pair,
                phasequad=False,
                rebin=False, freq="None")
        return selected_workspaces

    def _get_selected_groups_and_pairs(self):
        return self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs



