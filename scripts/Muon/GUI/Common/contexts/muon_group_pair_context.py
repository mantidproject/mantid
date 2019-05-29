# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.utilities.xml_utils as xml_utils
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair

from mantid.api import WorkspaceGroup
from mantid.kernel import ConfigServiceImpl
from Muon.GUI.Common.observer_pattern import Observable


def get_default_grouping(workspace, instrument, main_field_direction):
    parameter_name = "Default grouping file"
    if instrument == "MUSR" or instrument == 'CHRONUS':
        parameter_name += " - " + main_field_direction
    try:
        if isinstance(workspace, WorkspaceGroup):
            grouping_file = workspace[0].getInstrument().getStringParameter(parameter_name)[0]
        else:
            grouping_file = workspace.getInstrument().getStringParameter(parameter_name)[0]
    except IndexError:
        return [], []
    instrument_directory = ConfigServiceImpl.Instance().getInstrumentDirectory()
    filename = instrument_directory + grouping_file
    new_groups, new_pairs, description = xml_utils.load_grouping_from_XML(filename)
    return new_groups, new_pairs


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
    def __init__(self, check_group_contains_valid_detectors=lambda x : True):
        self._groups = []
        self._pairs = []

        self.message_notifier = MessageNotifier(self)

        self._check_group_contains_valid_detectors = check_group_contains_valid_detectors

    def __getitem__(self, name):
        for item in self._groups + self.pairs:
            if item.name == name:
                return item
        return None

    @property
    def groups(self):
        return self._groups

    @property
    def pairs(self):
        return self._pairs

    def clear_groups(self):
        self._groups = []

    def clear_pairs(self):
        self._pairs = []

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
        assert isinstance(pair, MuonPair)
        if self._check_name_unique(pair.name):
            self._pairs.append(pair)
        else:
            raise ValueError('Groups and pairs must have unique names')

    def show(self, name, run):
        self[name].show(str(run))

    def reset_group_and_pairs_to_default(self, workspace, instrument, main_field_direction):
        import pydevd
        pydevd.settrace('localhost', port=5534, stdoutToServer=True, stderrToServer=True)
        self._groups, self._pairs = get_default_grouping(workspace, instrument, main_field_direction)

    def _check_name_unique(self, name):
        for item in self._groups + self.pairs:
            if item.name == name:
                return False
        return True

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
        for item in self._groups + self._pairs:
            equivalent_name = item.get_rebined_or_unbinned_version_of_workspace_if_it_exists(workspace_name)
            if equivalent_name:
                return equivalent_name

        return None
