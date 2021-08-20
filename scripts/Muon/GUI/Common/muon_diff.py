# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from Muon.GUI.Common.muon_base import MuonBase
from Muon.GUI.Common.muon_base import MuonRun
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


GROUP = "group"


class MuonDiff(MuonBase):
    """
    Simple structure to store information on a difference.
    - The name is set at initialization and after that cannot be changed.
    - The difference has two properies positive and negative used in the calculation
    - The difference can be between two groups or two pairs only
    - The workspace associated to the difference can be set, but must be of type MuonWorkspaceWrapper.
    """

    def __init__(self, diff_name, positive, negative, group_or_pair=GROUP, periods=[1]):
        super(MuonDiff, self).__init__(diff_name, periods)
        self._positive = positive
        self._negative = negative
        self._group_or_pair = group_or_pair
        self._asymmetry_unnormalised = {}
        self._asymmetry_rebin_unnormalised = {}

    @property
    def positive(self):
        return self._positive

    @property
    def negative(self):
        return self._negative

    @property
    def group_or_pair(self):
        return self._group_or_pair

    def update_unnormalised_asymmetry_workspace(
            self,
            asymmetry_workspace,
            run,
            rebin=False):
        run_object = MuonRun(run)
        if not rebin and self.group_or_pair==GROUP:
            self._asymmetry_unnormalised.update(
                {run_object: MuonWorkspaceWrapper(asymmetry_workspace)})
        elif self.group_or_pair==GROUP:
            self._asymmetry_rebin_unnormalised.update(
                {run_object: MuonWorkspaceWrapper(asymmetry_workspace)})

    def find_unormalised(self, workspace):
        for key, value in self.workspace.items():
            if value.workspace_name == workspace:
                return self._asymmetry_unnormalised[key].workspace_name

        for key, value in self.workspace_rebin.items():
            if value.workspace_name == workspace:
                return self._asymmetry_rebin_unnormalised[key].workspace_name
