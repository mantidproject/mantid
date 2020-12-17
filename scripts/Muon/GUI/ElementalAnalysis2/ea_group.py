# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.muon_group import MuonRun
from typing import List


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
        self._counts_workspace = {}
        self._counts_workspace_rebin = {}

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

    """
    Returns the name of the counts workspace for a given run
    if the workspace does not exist will raise a KeyError
    """
    def get_counts_workspace_for_run(self, run, rebin):
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

    @property
    def periods(self):
        return self._periods

    @periods.setter
    def periods(self, value):
        self._periods = value

    def show_raw(self, run: List[int], name: str):
        run_object = MuonRun(run)
        run_object not in self._counts_workspace or self._counts_workspace[run_object].show(name)
