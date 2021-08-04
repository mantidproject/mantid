# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from math import floor
import Muon.GUI.Common.utilities.xml_utils as xml_utils
from Muon.GUI.Common.muon_diff import MuonDiff
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_phasequad import MuonPhasequad
from Muon.GUI.Common.muon_base_pair import MuonBasePair

from mantid.api import WorkspaceGroup
from mantid.kernel import ConfigServiceImpl
from mantidqt.utils.observer_pattern import Observable


def get_incremental_number_for_value_in_list(name, list_copy, current_number=1):
    if name + str(current_number) in list_copy:
        return get_incremental_number_for_value_in_list(name, list_copy, current_number + 1)
    else:
        return str(current_number)


def get_grouping_psi(workspace):
    grouping_list = []
    sample_log_value_list = []
    for ii in range(0, workspace.getNumberHistograms()):
        sample_log_label_name = "Label Spectra " + str(ii)
        workspace_run = workspace.getRun()

        if workspace_run.hasProperty(sample_log_label_name):
            sample_log_value = workspace_run.getProperty(sample_log_label_name).value
            if sample_log_value not in sample_log_value_list:
                grouping_list.append(MuonGroup(sample_log_value, [ii + 1]))
            else:
                sample_log_value += get_incremental_number_for_value_in_list(sample_log_value, sample_log_value_list)
                grouping_list.append(MuonGroup(sample_log_value, [ii + 1]))
            sample_log_value_list.append(sample_log_value)

    return grouping_list, [], [], ''


def get_default_grouping(workspace, instrument, main_field_direction):
    parameter_name = "Default grouping file"
    if instrument == "MUSR" or instrument == 'CHRONUS':
        parameter_name += " - " + main_field_direction

    if instrument != "PSI":
        try:
            if isinstance(workspace, WorkspaceGroup):
                grouping_file = workspace[0].getInstrument().getStringParameter(parameter_name)[0]
            else:
                grouping_file = workspace.getInstrument().getStringParameter(parameter_name)[0]

        except IndexError:
            return [], [], [], ''
    else:
        return get_grouping_psi(workspace)
    instrument_directory = ConfigServiceImpl.Instance().getInstrumentDirectory()
    filename = os.path.join(instrument_directory, grouping_file)
    new_groups, new_pairs, new_diffs, description, default = xml_utils.load_grouping_from_XML(filename)
    return new_groups, new_pairs, new_diffs, default


def construct_empty_group(group_names, group_index=0):
    """
    Create an empty MuonGroup appropriate for adding to the current grouping table.
    """
    new_group_name = "group_" + str(group_index)
    while new_group_name in group_names:
        # modify the name until it is unique
        group_index += 1
        new_group_name = "group_" + str(group_index)
    return MuonGroup(group_name=new_group_name, detector_ids=[1])


def construct_empty_pair(group_names, pair_names, pair_index=0):
    """
    Create an empty MuonPair appropriate for adding to the current pairing table.
    """
    new_pair_name = "pair_" + str(pair_index)
    while new_pair_name in pair_names:
        # modify the name until it is unique
        pair_index += 1
        new_pair_name = "pair_" + str(pair_index)
    if len(group_names) == 1:
        group1 = group_names[0]
        group2 = group_names[0]
    elif len(group_names) >= 2:
        group1 = group_names[0]
        group2 = group_names[1]
    else:
        group1 = None
        group2 = None
    return MuonPair(pair_name=new_pair_name,
                    forward_group_name=group1, backward_group_name=group2, alpha=1.0)


class MessageNotifier(Observable):
    def __init__(self, outer):
        Observable.__init__(self)
        self.outer = outer  # handle to containing class

    def notify_subscribers(self, *args, **kwargs):
        Observable.notify_subscribers(self, *args)


class MuonGroupPairContext(object):
    def __init__(self, check_group_contains_valid_detectors=lambda x: True):
        self._groups = []
        self._pairs = []
        self._diffs = []
        self._phasequad = []
        self._selected = ''
        self._selected_type = ''
        self._selected_pairs = []
        self._selected_groups = []
        self._selected_diffs = []

        self.message_notifier = MessageNotifier(self)

        self._check_group_contains_valid_detectors = check_group_contains_valid_detectors

    def __getitem__(self, name):
        for item in self.all_groups_and_pairs:
            if item.name == name:
                return item
        return None

    @property
    def groups(self):
        return self._groups

    @property
    def pairs(self):
        return self._pairs

    @property
    def phasequads(self):
        return self._phasequad

    @property
    def diffs(self):
        return self._diffs

    def get_diffs(self, group_or_pair):
        return [diff for diff in self._diffs if diff.group_or_pair == group_or_pair]

    @property
    def all_groups_and_pairs(self):
        return self.groups + self.pairs + self.diffs

    @property
    def selected_groups(self) -> list:
        """Returns the selected group names. Ensures the order of the returned group names is correct."""
        return [group.name for group in self.groups if group.name in self._selected_groups]

    @property
    def selected_pairs(self) -> list:
        """Returns the selected pair names. Ensures the order of the returned pair names is correct."""
        return [pair.name for pair in self.pairs if pair.name in self._selected_pairs]

    @property
    def selected_diffs(self) -> list:
        """Returns the selected diff names. Ensures the order of the returned diff names is correct."""
        return [diff.name for diff in self.diffs if diff.name in self._selected_diffs]

    @property
    def selected_groups_and_pairs(self):
        return self.selected_groups + self.selected_pairs + self.selected_diffs

    def clear(self):
        self.clear_groups()
        self.clear_pairs()

    def clear_groups(self):
        self._groups = []

    def clear_pairs(self):
        self._pairs = []

    def clear_diffs(self, group_or_pair):
        to_rm = []
        for diff in self._diffs:
            if diff.group_or_pair == group_or_pair:
                to_rm.append(diff.name)
        for name in to_rm:
            self.remove_diff(name)

    def clear_selected_pairs(self):
        self._selected_pairs = []

    def clear_selected_groups(self):
        self._selected_groups = []

    def clear_selected_diffs(self):
        self._selected_diffs = []

    @property
    def selected(self):
        return self._selected

    @property
    def selected_type(self):
        return self._selected_type

    @selected.setter
    def selected(self, value):
        if value in self.group_names + self.pair_names and self._selected != value:
            self._selected = value

    @selected_type.setter
    def selected_type(self, value):
        if value in ["Pair", "Group"] and self._selected_type != value:
            self._selected_type = value

    @property
    def diff_names(self):
        return [diff.name for diff in self._diffs]

    @property
    def group_names(self):
        return [group.name for group in self._groups]

    @property
    def pair_names(self):
        return [pair.name for pair in self._pairs]

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        if self._check_group_contains_valid_detectors(group):
            if self._check_name_unique(group.name):
                self._groups.append(group)
            else:
                raise ValueError('Groups and pairs must have unique names')
        else:
            raise ValueError('Invalid detectors in group {}'.format(group.name))

    def remove_group(self, group_name):
        for group in self._groups:
            if group.name == group_name:
                self._groups.remove(group)
                return

    def remove_pair(self, pair_name):
        for pair in self._pairs:
            if pair.name == pair_name:
                self._pairs.remove(pair)
                return

    def add_pair(self, pair):
        if isinstance(pair, MuonPair) and self._check_name_unique(pair.name):
            self._pairs.append(pair)
        elif isinstance(pair, MuonBasePair) and self._check_name_unique(pair.name):
            self._pairs.append(pair)
        else:
            raise ValueError('Groups and pairs must have unique names')

    def add_phasequad(self, phasequad):
        if isinstance(
            phasequad, MuonPhasequad) and self._check_name_unique(
            phasequad.Re.name) and self._check_name_unique(
                phasequad.Im.name):
            self._phasequad.append(phasequad)
            self.add_pair(phasequad.Re)
            self.add_pair(phasequad.Im)
        else:
            raise ValueError('Groups and pairs must have unique names')

    def remove_phasequad(self, phasequad_obj):
        for phasequad in self._phasequad:
            if phasequad.name == phasequad_obj.name:
                if phasequad_obj.Re in self._pairs:
                    self._pairs.remove(phasequad_obj.Re)
                if phasequad_obj.Im in self._pairs:
                    self._pairs.remove(phasequad_obj.Im)
                if phasequad_obj in self._phasequad:
                    self._phasequad.remove(phasequad_obj)
                return

    def update_phase_tables(self, table):
        for index, _ in enumerate(self._phasequad):
            self._phasequad[index].phase_table = table

    def add_diff(self, diff):
        assert isinstance(diff, MuonDiff)
        if self._check_name_unique(diff.name):
            self._diffs.append(diff)
        else:
            raise ValueError('Groups and pairs must have unique names')

    def remove_diff(self, diff_name):
        for diff in self._diffs:
            if diff.name == diff_name:
                self._diffs.remove(diff)
                return

    def reset_group_and_pairs_to_default(self, workspace, instrument, main_field_direction, num_periods):
        default_groups, default_pairs, default_diffs, default_selected = get_default_grouping(workspace, instrument, main_field_direction)
        if num_periods == 1:
            self._groups = default_groups
            self._diffs = default_diffs
            self._pairs = default_pairs
            self._selected = default_selected
        else:
            periods = range(num_periods + 1)[1:]
            self._groups = []
            self._diffs = []
            self._pairs = []
            for period in periods:
                for group in default_groups:
                    self._groups.append(MuonGroup(group.name + str(period), group.detectors, [period]))

            for period in periods:
                for pair in default_pairs:
                    self._pairs.append(MuonPair(pair.name + str(period), pair.forward_group + str(period),
                                       pair.backward_group + str(period), pair.alpha, [period]))

            if default_diffs:
                for diff in default_diffs:
                    self._diffs.append(MuonDiff(diff.name, diff.forward, diff.backward, diff.group_or_pair, diff.periods))
            else:
                for index in range(0, floor(len(periods)/2.)):
                    for pair in default_pairs:
                        odd_period = index*2 + 1
                        even_period = odd_period+1
                        self._diffs.append(MuonDiff("pair_diff"+str(index+1),
                                                    pair.name + str(odd_period), pair.name + str(even_period), group_or_pair="pair",
                                                    periods=[odd_period,even_period]))

            self._selected = self.pair_names[0]

    def _check_name_unique(self, name):
        for item in self.all_groups_and_pairs:
            if item.name == name:
                return False
        return True

    def get_group_counts_workspace_names(self, runs, groups, rebin=False):
        workspace_names = []
        for group_name in groups:
            try:
                name = self[group_name].get_counts_workspace_for_run(runs, rebin)
                workspace_names.append(name)
            except KeyError:
                continue
        return workspace_names

    def get_group_workspace_names(self, runs, groups, rebin):
        workspace_list = []

        for group_name in groups:
            group = self[group_name]
            if rebin:
                sub_list = group.get_asymmetry_workspace_names_rebinned(runs)
            else:
                sub_list = group.get_asymmetry_workspace_names(runs)

            workspace_list += sub_list

        return workspace_list

    def get_pair_workspace_names(self, runs, pairs, rebin):
        workspace_list = []

        for pair_name in pairs:
            pair = self[pair_name]
            if rebin:
                sub_list = pair.get_asymmetry_workspace_names_rebinned(runs)
            else:
                sub_list = pair.get_asymmetry_workspace_names(runs)

            workspace_list += sub_list

        return workspace_list

    def get_equivalent_group_pair(self, workspace_name):
        for item in self.all_groups_and_pairs:
            equivalent_name = item.get_rebined_or_unbinned_version_of_workspace_if_it_exists(workspace_name)
            if equivalent_name:
                return equivalent_name

        return None

    # selected groups to analyse
    def set_selected_groups_to_all(self):
        self.clear_groups()
        for group in self.group_names:
            self._selected_groups.append(group)

    def reset_selected_groups_and_pairs(self):
        self.clear_selected_pairs()
        self.clear_selected_groups()
        self.clear_selected_diffs()

    def add_group_to_selected_groups(self, group):
        if group in self.group_names and group not in self.selected_groups:
            self._selected_groups.append(str(group))

    def remove_group_from_selected_groups(self, group):
        if group in self.group_names and group in self.selected_groups:
            self._selected_groups.remove(str(group))

    def add_diff_to_selected_diffs(self, diff):
        if diff in self.diff_names and diff not in self.selected_diffs:
            self._selected_diffs.append(str(diff))

    def remove_diff_from_selected_diffs(self, diff):
        if diff in self.diff_names and diff in self.selected_diffs:
            self._selected_diffs.remove(str(diff))

    def add_pair_to_selected_pairs(self, pair):
        if pair in self.pair_names and pair not in self.selected_pairs:
            self._selected_pairs.append(str(pair))

    def remove_pair_from_selected_pairs(self, pair):
        if pair in self.pair_names and pair in self.selected_pairs:
            self._selected_pairs.remove(str(pair))

    def remove_workspace_by_name(self, workspace_name):
        for item in self.all_groups_and_pairs:
            item.remove_workspace_by_name(workspace_name)

    def get_unormalisised_workspace_list(self, workspace_list):
        return [self.find_unormalised_workspace(workspace) for workspace in workspace_list]

    def find_unormalised_workspace(self, workspace):
        for group in self.groups:
            unnormalised_workspace = group.find_unormalised(workspace)
            if unnormalised_workspace:
                return unnormalised_workspace

    def get_group_pair_name_and_run_from_workspace_name(self, workspace_name):
        for group_pair in self.all_groups_and_pairs:
            run = group_pair.get_run_for_workspace(workspace_name)
            if(run):
                return group_pair.name, str(run)

        return None, None
