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
        self._counts_workspace = {}
        self._asymmetry_estimate = {}
        self._counts_workspace_rebin = {}
        self._asymmetry_estimate_rebin = {}

    @property
    def workspace(self):
        return self._counts_workspace

    @workspace.setter
    def workspace(self, new_workspace):
        if isinstance(new_workspace, MuonWorkspaceWrapper):
            self._counts_workspace = new_workspace
        else:
            raise AttributeError("Attempting to set workspace to type " + str(
                type(new_workspace)) + " but should be MuonWorkspaceWrapper")

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
                self._detector_ids = sorted(list(set(detector_ids)))
            else:
                raise AttributeError("MuonGroup : detectors must be a list of ints.")
        else:
            raise ValueError("detectors must be a list of ints.")

    def show_raw(self, run, name, asym_name):
        str(run) not in self._counts_workspace or self._counts_workspace[str(run)].show(name)
        str(run) not in self._asymmetry_estimate or self._asymmetry_estimate[str(run)].show(asym_name)

    def show_rebin(self, run, name, asym_name):
        str(run) not in self._counts_workspace_rebin or self._counts_workspace_rebin[str(run)].show(name)
        str(run) not in self._asymmetry_estimate_rebin or self._asymmetry_estimate_rebin[str(run)].show(asym_name)

    def update_workspaces(self, run, counts_workspace, asymmetry_workspace, rebin):
        if rebin:
            self._counts_workspace_rebin.update({str(run): MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate_rebin.update({str(run): MuonWorkspaceWrapper(asymmetry_workspace)})
        else:
            self._counts_workspace.update({str(run): MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate.update({str(run): MuonWorkspaceWrapper(asymmetry_workspace)})

    def update_counts_workspace(self, counts_workspace, run):
        self._counts_workspace.update({run: MuonWorkspaceWrapper(counts_workspace)})

    def get_asymmetry_workspace_names(self, runs):
        workspace_list = []

        for run in runs:
            if str(run) in self._asymmetry_estimate and self._asymmetry_estimate[str(run)].workspace_name and not \
                    self._asymmetry_estimate[str(run)].is_hidden:
                workspace_list.append(self._asymmetry_estimate[str(run)].workspace_name)

        return workspace_list

    def get_asymmetry_workspace_names_rebinned(self, runs):
        workspace_list = []

        for run in runs:
            if str(run) in self._asymmetry_estimate_rebin and self._asymmetry_estimate_rebin[str(run)].workspace_name \
                    and not self._asymmetry_estimate_rebin[str(run)].is_hidden:

                workspace_list.append(self._asymmetry_estimate_rebin[str(run)].workspace_name)

        return workspace_list

    def get_rebined_or_unbinned_version_of_workspace_if_it_exists(self, name):
        for key, value in self._asymmetry_estimate.items():
            if value.workspace_name == name and key in self._asymmetry_estimate_rebin:
                return self._asymmetry_estimate_rebin[key].workspace_name

        for key, value in self._counts_workspace.items():
            if value.workspace_name == name and key in self._counts_workspace_rebin:
                return self._counts_workspace_rebin[key].workspace_name

        for key, value in self._asymmetry_estimate_rebin.items():
            if value.workspace_name == name and key in self._asymmetry_estimate:
                return self._asymmetry_estimate[key].workspace_name

        for key, value in self._counts_workspace_rebin.items():
            if value.workspace_name == name and key in self._counts_workspace:
                return self._counts_workspace[key].workspace_name

        return None

    def remove_workspace_by_name(self, workspace_name):
        """
        Searches through all of the stored workspaces and remmves any which match the name given. This is used to handle
        workspaces being removed from the ADS.
        :param workspace_name:
        :return:

        """

        def _remove_workspace_from_dict_by_name(workspace_name, dictionary):
            set_of_keys_to_remove = set()
            for key, workspace_wrapper in dictionary.items():
                if workspace_wrapper.workspace_name == workspace_name:
                    set_of_keys_to_remove.add(key)

            for key in set_of_keys_to_remove:
                dictionary.pop(key)

        _remove_workspace_from_dict_by_name(workspace_name, self._counts_workspace)
        _remove_workspace_from_dict_by_name(workspace_name, self._asymmetry_estimate)
        _remove_workspace_from_dict_by_name(workspace_name, self._counts_workspace_rebin)
        _remove_workspace_from_dict_by_name(workspace_name, self._asymmetry_estimate_rebin)
