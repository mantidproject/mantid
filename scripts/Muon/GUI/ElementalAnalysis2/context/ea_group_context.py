# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from mantidqt.utils.observer_pattern import GenericObservable

INVALID_STRINGS_FOR_GROUP_NAMES = ["EA_Rebinned_Fixed", "EA_Rebinned_Variable", "peaks", "refitted_peaks",
                                   "with_errors", "matches"]


def get_default_grouping(loadedData):
    """this creates the first set of groups listed in the grouping table of the Elemental Analysis GUI
        For single workspace names the detector is found by taking everything after ; in the name
        For example : 2695; Detector 1 --> Detector 1
        For co-added workspaces the detector is found by taking everything before
        For example : Detector 1_2695-2686 --> Detector 1
    """
    groups = []
    run_list = loadedData.get_parameter("run")
    for run_item in run_list:
        for workspace in loadedData.get_data(run=run_item)["workspace"]:
            group_name = str(workspace)
            if not is_group_valid(group_name):
                continue
            detector_name = (group_name.split(';', 1)[-1].lstrip()).split('_', 1)[0]
            run_number = str(run_item).replace('[', '').replace(']', '')
            groups += [EAGroup(group_name=group_name, detector=detector_name, run_number=run_number)]
    return groups


def is_group_valid(group_name):
    """
        format of group name is run; Detector x , where x is an integer between 1 and 4 inclusively, if workspace has
        anything else at the end it may be invalid so function checks if trailing string makes name is invalid
    """
    for suffix in INVALID_STRINGS_FOR_GROUP_NAMES:
        if group_name.endswith(suffix):
            return False
    return True


class EAGroupContext(object):
    def __init__(self, check_group_contains_valid_detectors=lambda x: True):
        self._groups = []
        self._runs_in_groups = []
        self._selected = ''
        self._selected_type = ''
        self._selected_groups = []

        self.message_notifier = GenericObservable()

        self._check_group_contains_valid_detectors = check_group_contains_valid_detectors

    def __getitem__(self, name):
        for item in self._groups:
            if item.name == name:
                return item
        return None

    @property
    def groups(self):
        return self._groups

    @property
    def selected_groups(self):
        return self._selected_groups

    def clear(self):
        self._groups = []

    def clear_selected_groups(self):
        self._selected_groups = []

    @property
    def group_names(self):
        return [group.name for group in self._groups]

    def add_new_group(self, group, loadedData):
        """this adds groups to the grouping tab that are not already loaded"""
        run_list = loadedData.get_parameter("run")
        for run_item in run_list:
            for workspace in loadedData.get_data(run=run_item)["workspace"]:
                if str(workspace) not in self.group_names:
                    group_name = str(workspace)
                    # For single workspace names the detector is found by taking everything after ; in the name
                    # For co-added workspaces the detector is found by taking everything before _
                    if not is_group_valid(group_name):
                        continue
                    detector_name = (group_name.split(';', 1)[-1].lstrip()).split('_', 1)[0]
                    run_number = str(run_item).replace('[', '').replace(']', '')
                    group += [EAGroup(group_name=group_name, detector=detector_name, run_number=run_number)]
        return group

    def add_group(self, group):
        """this adds groups to the grouping tab that are not already loaded"""
        if group._group_name not in self.group_names and is_group_valid(group._group_name):
            self._groups.append(group)
        return group

    def reset_group_to_default(self, loadedData):
        if loadedData:
            self._groups = get_default_grouping(loadedData)

    def add_group_to_selected_groups(self, group):
        if group in self.group_names and group not in self.selected_groups:
            self._selected_groups.append(str(group))

    def remove_group_from_selected_groups(self, group):
        if group in self.group_names and group in self.selected_groups:
            self._selected_groups.remove(str(group))

    def remove_group(self, group_name):
        for group in self._groups:
            if group.name == group_name:
                self._groups.remove(group)
                return
