# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from typing import List
import itertools


class MuonRun(object):
    """
    Holds the experimental runs a dataset corrosponds to.
    Due to the ability to co-add runs on load this can be one to many.
    This is used to index and referance by dataset.
    """
    def __init__(self, run_numbers: List[int]):
        self._runs = tuple(run_numbers)

    """
    A range of runs should be returned seperated by a dash whilst
    non-cosecutive runs should be comma seperated.
    For example (62260, 62261, 62263, 62270)
    should return '62260-62263, 62270'
    """
    def __str__(self):
        return run_list_to_string(list(self._runs))

    def __eq__(self, other):
        return isinstance(other, self.__class__) and other._runs == self._runs

    def __repr__(self):
        return 'MuonRun({})'.format(self._runs)

    """
    We need to be able to hash this class so it can be used as
    a key in dict objects. For this purpose just hashing the
    internal tuple is sufficient.
    """
    def __hash__(self):
        return hash(self._runs)


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
        self._asymmetry_estimate_unormalised = {}
        self._asymmetry_estimate_rebin_unormalised = {}

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
        if isinstance(detector_ids, str):
            raise AttributeError("MuonGroup : detectors must be a list of ints.")
        elif isinstance(detector_ids, list):
            if sum([not isinstance(item, int) for item in detector_ids]) == 0:
                self._detector_ids = sorted(list(set(detector_ids)))
            else:
                raise AttributeError("MuonGroup : detectors must be a list of ints.")
        else:
            raise ValueError("detectors must be a list of ints.")

    def show_raw(self, run: List[int], name: str, asym_name: str, asym_name_unnorm: str):
        run_object = MuonRun(run)
        run_object not in self._counts_workspace or self._counts_workspace[run_object].show(name)
        run_object not in self._asymmetry_estimate or self._asymmetry_estimate[run_object].show(asym_name)
        run_object not in self._asymmetry_estimate_unormalised or\
            self._asymmetry_estimate_unormalised[run_object].show(asym_name_unnorm)

    def show_rebin(self, run, name, asym_name, asym_name_unnorm):
        run_object = MuonRun(run)
        run_object not in self._counts_workspace_rebin or self._counts_workspace_rebin[run_object].show(name)
        run_object not in self._asymmetry_estimate_rebin or self._asymmetry_estimate_rebin[run_object].show(asym_name)
        run_object not in self._asymmetry_estimate_rebin_unormalised or \
            self._asymmetry_estimate_rebin_unormalised[run_object].show(asym_name_unnorm)

    def update_workspaces(self, run, counts_workspace, asymmetry_workspace, asymmetry_workspace_unnorm, rebin):
        run_object = MuonRun(run)
        if rebin:
            self._counts_workspace_rebin.update({run_object: MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate_rebin.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace)})
            self._asymmetry_estimate_rebin_unormalised.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace_unnorm)})
        else:
            self._counts_workspace.update({run_object: MuonWorkspaceWrapper(counts_workspace)})
            self._asymmetry_estimate.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace)})
            self._asymmetry_estimate_unormalised.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace_unnorm)})

    def update_counts_workspace(self, counts_workspace, run):
        self._counts_workspace.update({run: MuonWorkspaceWrapper(counts_workspace)})

    def get_asymmetry_workspace_names(self, runs):
        workspace_list = []

        for run in runs:
            run_object = MuonRun(run)
            if run_object in self._asymmetry_estimate and self._asymmetry_estimate[run_object].workspace_name and not \
                    self._asymmetry_estimate[run_object].is_hidden:
                workspace_list.append(self._asymmetry_estimate[run_object].workspace_name)

        return workspace_list

    def get_asymmetry_workspace_names_rebinned(self, runs):
        workspace_list = []

        for run in runs:
            run_object = MuonRun(run)
            if run_object in self._asymmetry_estimate_rebin and self._asymmetry_estimate_rebin[run_object].workspace_name \
                    and not self._asymmetry_estimate_rebin[run_object].is_hidden:

                workspace_list.append(self._asymmetry_estimate_rebin[run_object].workspace_name)

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

    def get_run_for_workspace(self, workspace_name):
        for key, value in itertools.chain(self._asymmetry_estimate.items(),
                                          self._counts_workspace.items(),
                                          self._asymmetry_estimate_rebin.items(),
                                          self._counts_workspace_rebin.items()):
            if value.workspace_name in workspace_name:
                return key

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

    def find_unormalised(self, workspace):
        for key, value in self._asymmetry_estimate.items():
            if value.workspace_name == workspace:
                return self._asymmetry_estimate_unormalised[key].workspace_name

        for key, value in self._asymmetry_estimate_rebin.items():
            if value.workspace_name == workspace:
                return self._asymmetry_estimate_rebin_unormalised[key].workspace_name
