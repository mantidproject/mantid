# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0111
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
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
        return "MuonRun({})".format(self._runs)

    """
    We need to be able to hash this class so it can be used as
    a key in dict objects. For this purpose just hashing the
    internal tuple is sufficient.
    """

    def __hash__(self):
        return hash(self._runs)


class MuonBase(object):
    """
    Simple structure to store information of a pair or group.

    - The name is set at initialization and after that cannot be changed.
    - The pair has two groups associated to it, and we store only their names.
    - The balance parameter is stored and modifiable.
    - The workspace associated to the pair can be set, but must be of type MuonWorkspaceWrapper.
    """

    def __init__(self, name, periods=[1]):
        self._name = name
        self._workspace = {}
        self.workspace_rebin = {}
        self._periods = periods

    @property
    def periods(self):
        return self._periods

    @property
    def workspace(self):
        return self._workspace

    @workspace.setter
    def workspace(self, new_workspace):
        if isinstance(new_workspace, MuonWorkspaceWrapper):
            self._workspace = new_workspace
        else:
            raise AttributeError("Attempting to set workspace to type " + str(type(new_workspace)) + " but should be MuonWorkspaceWrapper")

    @property
    def name(self):
        return self._name

    def show_raw(self, run, name):
        run_object = MuonRun(run)
        run_object not in self._workspace or self._workspace[run_object].show(name)

    def show_rebin(self, run, name):
        run_object = MuonRun(run)
        run_object not in self.workspace_rebin or self.workspace_rebin[run_object].show(name)

    def update_asymmetry_workspace(self, asymmetry_workspace, run, rebin=False):
        run_object = MuonRun(run)
        if not rebin:
            self._workspace.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace)})
        else:
            self.workspace_rebin.update({run_object: MuonWorkspaceWrapper(asymmetry_workspace)})

    def get_asymmetry_workspace_names(self, runs):
        workspace_list = []

        for run in runs:
            run_object = MuonRun(run)
            if run_object in self._workspace and self._workspace[run_object].workspace_name:
                workspace_list.append(self._workspace[run_object].workspace_name)

        return workspace_list

    def get_asymmetry_workspace_names_rebinned(self, runs):
        workspace_list = []
        for run in runs:
            run_object = MuonRun(run)
            if run_object in self.workspace_rebin and self.workspace_rebin[run_object].workspace_name:
                workspace_list.append(self.workspace_rebin[run_object].workspace_name)

        return workspace_list

    def get_rebined_or_unbinned_version_of_workspace_if_it_exists(self, name):
        for key, value in self._workspace.items():
            if value.workspace_name == name and key in self.workspace_rebin:
                return self.workspace_rebin[key].workspace_name

        for key, value in self.workspace_rebin.items():
            if value.workspace_name == name and key in self._workspace:
                return self._workspace[key].workspace_name

        return None

    def get_run_for_workspace(self, workspace_name):
        for key, value in itertools.chain(self._workspace.items(), self.workspace_rebin.items()):
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

        _remove_workspace_from_dict_by_name(workspace_name, self._workspace)
        _remove_workspace_from_dict_by_name(workspace_name, self.workspace_rebin)
