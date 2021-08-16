# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.ElementalAnalysis2.context.context import REBINNED_VARIABLE_WS_SUFFIX, REBINNED_FIXED_WS_SUFFIX
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import PEAKS_WS_SUFFIX, REFITTED_PEAKS_WS_SUFFIX,\
    MATCH_GROUP_WS_SUFFIX, ERRORS_WS_SUFFIX


INVALID_STRINGS_FOR_GROUP_NAMES = [REBINNED_VARIABLE_WS_SUFFIX, REBINNED_FIXED_WS_SUFFIX, PEAKS_WS_SUFFIX,
                                   REFITTED_PEAKS_WS_SUFFIX, ERRORS_WS_SUFFIX, MATCH_GROUP_WS_SUFFIX]


def get_default_grouping(loadedData, error_notifier=None):
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
            group = EAGroup(group_name=group_name, detector=detector_name, run_number=run_number)
            if error_notifier is not None:
                group.set_error_notifier(error_notifier)
            groups += [group]
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
    def __init__(self, check_group_contains_valid_detectors=lambda x: True, error_notifier=None):
        self._groups = []
        self._runs_in_groups = []
        self._selected = ''
        self._selected_type = ''
        self._selected_groups = []

        self.message_notifier = GenericObservable()
        self.error_notifier = error_notifier
        self._check_group_contains_valid_detectors = check_group_contains_valid_detectors

    def __getitem__(self, name):
        for item in self._groups:
            if item.name == name:
                return item
        return None

    @property
    def groups(self):
        return self._groups

    def set_error_notifier(self, notifier: GenericObservable):
        self.error_notifier = notifier

    @property
    def selected_groups(self):
        return self._selected_groups

    def clear(self):
        self._groups = []

    def clear_selected_groups(self):
        self._selected_groups = []

    def create_EAGroup(self, group_name, detector, run_number):
        # This method should be used to create EAGroups as it sets error notifier
        group = EAGroup(group_name=group_name, detector=detector, run_number=run_number)
        group.set_error_notifier(self.error_notifier)
        return group

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
                    group += [self.create_EAGroup(group_name=group_name, detector=detector_name, run_number=run_number)]
        return group

    def add_group(self, group):
        """this adds groups to the grouping tab that are not already loaded"""
        if group._group_name not in self.group_names and is_group_valid(group._group_name):
            self._groups.append(group)
        return group

    def reset_group_to_default(self, loadedData):
        if loadedData:
            self._groups = get_default_grouping(loadedData, self.error_notifier)

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

    @staticmethod
    def get_detector_and_run_from_workspace_name(workspace_name):
        for suffix in INVALID_STRINGS_FOR_GROUP_NAMES:
            if suffix in workspace_name:
                workspace_name = workspace_name.replace(suffix, "")

        run, detector = workspace_name.split(";")

        return run.strip(), detector.strip()

    def remove_workspace_from_group(self, workspace_name):
        """
        handles removing workspace from group
        :param workspace_name : name of workspace removed
        """
        for name in [REBINNED_FIXED_WS_SUFFIX, REBINNED_VARIABLE_WS_SUFFIX, PEAKS_WS_SUFFIX, MATCH_GROUP_WS_SUFFIX]:
            if workspace_name.endswith(name):
                group_name = workspace_name.replace(name, "")
                group = self[group_name]
                if group is None:
                    return
                if workspace_name.endswith(REBINNED_FIXED_WS_SUFFIX):
                    group.remove_rebinned_workspace()

                elif workspace_name.endswith(REBINNED_VARIABLE_WS_SUFFIX):
                    group.remove_rebinned_workspace()

                elif workspace_name.endswith(PEAKS_WS_SUFFIX):
                    group.remove_peak_table()

                elif workspace_name.endswith(MATCH_GROUP_WS_SUFFIX):
                    group.remove_matches_group()
