# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.muon_group import MuonRun


class EAGroup(object):
    """
    Simple structure to store information on a group in Elemental Analysis.

    - The name is set at initialization and after that cannot be changed.
    - The detector list can be modified by passing a list of ints (type checks for this).
    - The number of detectors is stored.
    - The workspace associated to the group can be set, but must be of type MuonWorkspaceWrapper.
    """

    def __init__(self, group_name, detector, run_number):
        self._group_name = group_name

        self.detector = detector
        self.run_number = run_number
        self.rebin_index = 0
        self.rebin_option = None
        self._counts_workspace = {}
        self._counts_workspace_rebin = {}
        self._peak_table = {}
        self._matches_table = {}
        self.update_counts_workspace(str(group_name), run_number)

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

    def get_counts_workspace_for_run(self, run, rebin):
        """
            Returns the name of the counts workspace for a given run
            if the workspace does not exist will raise a KeyError
        """
        if rebin:
            return self._counts_workspace_rebin[MuonRun(run)].workspace_name
        else:
            return self._counts_workspace[MuonRun(run)].workspace_name

    @property
    def name(self):
        return self._group_name

    @name.setter
    def name(self, name):
        raise AttributeError("Attempting to change name from {} to {}. "
                             "Cannot change name of EAGroup "
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

    def update_workspaces(self, run, counts_workspace, rebin):
        run_object = MuonRun(run)
        if rebin:
            self._counts_workspace_rebin.update({run_object: MuonWorkspaceWrapper(counts_workspace)})
        else:
            self._counts_workspace.update({run_object: MuonWorkspaceWrapper(counts_workspace)})

    def update_counts_workspace(self, counts_workspace, run):
        run_object = MuonRun(run)
        self._counts_workspace.update({run_object: MuonWorkspaceWrapper(counts_workspace)})

    def get_rebined_or_unbinned_version_of_workspace_if_it_exists(self, name):
        for key, value in self._counts_workspace.items():
            if value.workspace_name == name and key in self._counts_workspace_rebin:
                return self._counts_workspace_rebin[key].workspace_name

        for key, value in self._counts_workspace_rebin.items():
            if value.workspace_name == name and key in self._counts_workspace:
                return self._counts_workspace[key].workspace_name

        return None

    def update_peak_table(self, run, table):
        run_object = MuonRun(run)
        self._peak_table.update({run_object: MuonWorkspaceWrapper(table)})

    def update_matches_table(self, run, table):
        run_object = MuonRun(run)
        self._matches_table.update({run_object: MuonWorkspaceWrapper(table)})

    def get_peak_table(self, run):
        run_object = MuonRun(run)
        return self._peak_table[run_object].workspace_name

    def get_matches_table(self, run):
        run_object = MuonRun(run)
        return self._matches_table[run_object].workspace_name

    def is_peak_table_present(self):
        return bool(self._peak_table)

    def is_matches_table_present(self):
        return bool(self._matches_table)
