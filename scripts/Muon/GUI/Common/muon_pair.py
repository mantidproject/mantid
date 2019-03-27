# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


class MuonPair(object):
    """
    Simple structure to store information on a detector group pair.

    - The name is set at initialization and after that cannot be changed.
    - The pair has two groups associated to it, and we store only their names.
    - The balance parameter is stored and modifiable.
    - The workspace associated to the pair can be set, but must be of type MuonWorkspaceWrapper.
    """

    def __init__(self, pair_name,
                 forward_group_name="",
                 backward_group_name="",
                 alpha=1.0):

        self._pair_name = pair_name
        self._forward_group_name = forward_group_name
        self._backward_group_name = backward_group_name
        self._alpha = float(alpha)
        self._workspace = {}
        self.workspace_rebin = {}

    @property
    def workspace(self):
        return self._workspace

    @workspace.setter
    def workspace(self, new_workspace):
        if isinstance(new_workspace, MuonWorkspaceWrapper):
            self._workspace = new_workspace
        else:
            raise AttributeError("Attempting to set workspace to type " + str(type(new_workspace)) +
                                 " but should be MuonWorkspaceWrapper")

    @property
    def name(self):
        return self._pair_name

    @property
    def forward_group(self):
        return self._forward_group_name

    @property
    def backward_group(self):
        return self._backward_group_name

    @forward_group.setter
    def forward_group(self, new_name):
        self._forward_group_name = new_name

    @backward_group.setter
    def backward_group(self, new_name):
        self._backward_group_name = new_name

    @property
    def alpha(self):
        return float("{}".format(self._alpha))

    @alpha.setter
    def alpha(self, new_alpha):
        if float(new_alpha) >= 0.0:
            self._alpha = float(new_alpha)
        else:
            raise AttributeError("Alpha must be > 0.0.")

    def show_raw(self, run, name):
        str(run) not in self._workspace or self._workspace[str(run)].show(name)

    def show_rebin(self, run, name):
        str(run) not in self.workspace_rebin or self.workspace_rebin[str(run)].show(name)

    def update_asymmetry_workspace(self, asymmetry_workspace, run, rebin=False):
        if not rebin:
            self._workspace.update({str(run): MuonWorkspaceWrapper(asymmetry_workspace)})
        else:
            self.workspace_rebin.update({str(run): MuonWorkspaceWrapper(asymmetry_workspace)})
