# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
import six


class MuonGroup(object):
    """
    Simple structure to store information on a detector group.

    - The name is set at initialization and after that cannot be changed.
    - The detector list can be modified by passing a list of ints (type checks for this).
    - The number of detectors is stored.
    - The workspace associated to the group can be set, but must be of type MuonWorkspaceWrapper.
    """

    def __init__(self, group_name, detector_ids=[]):

        self._group_name = group_name
        self._detector_ids = None
        self.detectors = detector_ids
        self._workspace = {}
        self._asymmetry_estimate = {}
        self._workspace_rebin = {}
        self._asymmetry_estimate_rebin = {}

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
        return self._group_name

    @name.setter
    def name(self, name):
        raise AttributeError("Attempting to change name from {} to {}. "
                             "Cannot change name of MuonGroup "
                             "object".format(self._group_name, name))

    @property
    def detectors(self):
        return self._detector_ids

    @property
    def n_detectors(self):
        return len(self.detectors)

    @detectors.setter
    def detectors(self, detector_ids):
        if isinstance(detector_ids, six.string_types):
            raise AttributeError("MuonGroup : detectors must be a list of ints.")
        elif isinstance(detector_ids, list):
            if sum([not isinstance(item, int) for item in detector_ids]) == 0:
                self._detector_ids = list(set(sorted(detector_ids)))
            else:
                raise AttributeError("MuonGroup : detectors must be a list of ints.")
        else:
            raise ValueError("detectors must be a list of ints.")

    def show(self, run):
        run not in self._workspace or self._workspace[run].show()
        run not in self._asymmetry_estimate or self._asymmetry_estimate[run].show()
        run not in self._workspace_rebin or self._workspace_rebin[run].show()
        run not in self._asymmetry_estimate_rebin or self._asymmetry_estimate_rebin[run].show()

    def update_workspaces(self, run, counts_workspace, asymmetry_workspace, rebin):
        if rebin:
            self._workspace_rebin.update({run: MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate_rebin.update({run: MuonWorkspaceWrapper(asymmetry_workspace)})
        else:
            self._workspace.update({run: MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate.update({run: MuonWorkspaceWrapper(asymmetry_workspace)})