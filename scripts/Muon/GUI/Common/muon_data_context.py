# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.load_utils as load_utils

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.muon_file_utils import format_run_for_file
from Muon.GUI.Common.run_string_utils import run_list_to_string
from Muon.Gui.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name, get_group_data_workspace_name,
                                                          get_pair_data_workspace_name, get_base_data_directory,
                                                          get_raw_data_directory, get_cached_data_directory,
                                                          get_group_data_directory, get_pair_data_directory)

from Muon.Gui.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data

from collections import OrderedDict

from mantid.kernel import ConfigServiceImpl, ConfigService


def get_default_grouping(instrument, main_field_direction):
    parameter_name = "Default grouping file"
    if instrument == "MUSR":
        parameter_name += " - " + main_field_direction
    try:
        grouping_file = ConfigService.getInstrument(instrument).getStringParameter(parameter_name)[0]
    except IndexError:
        return [], []
    instrument_directory = ConfigServiceImpl.Instance().getInstrumentDirectory()
    filename = instrument_directory + grouping_file
    new_groups, new_pairs = load_utils.load_grouping_from_XML(filename)
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


class MuonDataContext(object):
    """
    The MuonContext is the core class for the MuonAnalysis 2 interface. It stores all the data and parameters used
    in the interface and serves as the model part of the MVP design pattern for every widget in the interface.
    By sharing a common instance of this class, the interface remains synchronized by use of the observer pattern to
    notify subcribers of changes, whi will then respond by updating their view from this commonly shared model.

    The actual processing of data occurs via this class (as it should as the model).
    """

    # ADS base directory for all workspaces
    base_directory = "Muon Data"

    def __init__(self, load_data=MuonLoadData()):
        """
        Currently, only a single run is loaded into the Home/Grouping tab at once. This is held in the _current_data
        member. The load widget may load multiple runs at once, these are stored in the _loaded_data member.
        Groups and Pairs associated to the current run are stored in _grousp and _pairs as ordered dictionaries.
        """
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = load_data
        self._current_data = {"workspace": load_utils.empty_loaded_data()}  # self.get_result(False)

    def is_data_loaded(self):
        return self._loaded_data.num_items() > 0

    def is_multi_period(self):
        return isinstance(self.current_data["OutputWorkspace"], list)

    @property
    def current_data(self):
        return self._current_data["workspace"]

    @property
    def instrument(self):
        inst = ConfigService.getInstrument()
        return inst

    @property
    def current_run(self):
        return self._current_data["run"]

    @property
    def run(self):
        try:
            # refer to the output of the loading widget (e.g. for co-adding)
            runs = run_list_to_string(self.current_run)
        except Exception:
            # extract from sample logs
            run_log = self.get_sample_log("run_number")
            if run_log:
                runs = run_log.value
            else:
                runs = 0
        return runs

    @property
    def group_names(self):
        return self._groups.keys()

    @property
    def pair_names(self):
        return self._pairs.keys()

    @property
    def groups(self):
        return self._groups

    @property
    def pairs(self):
        return self._pairs

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        self._groups[group.name] = group

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._pairs[pair.name] = pair

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their default values
        if self._loaded_data.num_items() > 0:
            self._current_data = self._loaded_data.get_latest_data()  # self._loaded_data.params["workspace"][-1]
            self.set_groups_and_pairs_to_default()
        else:
            self._current_data = {"workspace": load_utils.empty_loaded_data()}

    @property
    def loaded_data(self):
        return self._current_data["workspace"]

    @property
    def loaded_workspace(self):
        if self.is_multi_period():
            # return the first workspace in the group
            return self.current_data["OutputWorkspace"][0].workspace
        else:
            return self.current_data["OutputWorkspace"].workspace

    @property
    def period_string(self):
        # Get the period string i.e. "1+2-3+4" to be used in workspace naming.
        return "1"

    @property
    def num_detectors(self):
        try:
            n_det = self.loaded_workspace.detectorInfo().size()
        except AttributeError:
            # default to 1
            n_det = 1
        return n_det

    @property
    def main_field_direction(self):
        return self.current_data["MainFieldDirection"]

    @property
    def dead_time_table(self):
        return self.loaded_data["DeadTimeTable"]

    def get_sample_logs(self):
        logs = None
        try:
            logs = self.loaded_workspace.getSampleDetails()
        except Exception:
            print("Cannot find sample logs")
        return logs

    def get_sample_log(self, log_name):
        logs = self.get_sample_logs()
        try:
            log = logs.getLogData(log_name)
        except Exception:
            log = None
        return log

    # ------------------------------------------------------------------------------------------------------------------
    # Clearing data
    # ------------------------------------------------------------------------------------------------------------------

    def clear_groups(self):
        self._groups = OrderedDict()

    def clear_pairs(self):
        self._pairs = OrderedDict()

    def clear(self):
        self.clear_groups()
        self.clear_pairs()
        self._current_data = {"workspace": load_utils.empty_loaded_data()}

    def _base_run_name(self):
        """ e.g. EMU0001234 """
        if isinstance(self.run, int):
            return str(self.instrument) + format_run_for_file(self.run)
        else:
            return str(self.instrument) + self.run

    # ------------------------------------------------------------------------------------------------------------------
    # Showing workspaces in the ADS
    # ------------------------------------------------------------------------------------------------------------------

    def show_raw_data(self):
        workspace = self.current_data["OutputWorkspace"]
        directory = get_base_data_directory(self) + get_raw_data_directory(self)

        if isinstance(workspace, list):
            # Multi-period data
            for i, single_ws in enumerate(workspace):
                name = directory + get_raw_data_workspace_name(self) + "_period_" + str(i)
                single_ws.show(name)
        else:
            # Single period data
            name = directory + get_raw_data_workspace_name(self)
            workspace.show(name)

    def show_all_groups(self):
        for group_name in self._groups.keys():
            self.show_group_data(group_name)

    def show_group_data(self, group_name, show=True):
        name = get_group_data_workspace_name(self, group_name)
        directory = get_base_data_directory(self) + get_group_data_directory(self)
        workspace = calculate_group_data(self, group_name)

        self._groups[group_name].workspace = load_utils.MuonWorkspace(workspace)
        if show:
            self._groups[group_name].workspace.show(directory + name)

    def show_all_pairs(self):
        for pair_name in self._pairs.keys():
            self.show_pair_data(pair_name)

    def show_pair_data(self, pair_name, show=True):
        name = get_pair_data_workspace_name(self, pair_name)
        directory = get_base_data_directory(self) + get_pair_data_directory(self)
        workspace = calculate_pair_data(self, pair_name)

        self._pairs[pair_name].workspace = load_utils.MuonWorkspace(workspace)
        if show:
            self._pairs[pair_name].workspace.show(directory + name)

    def calculate_all_groups(self):
        for group_name in self._groups.keys():
            calculate_group_data(group_name)

    def set_groups_and_pairs_to_default(self):
        groups, pairs = get_default_grouping(self.instrument, self.main_field_direction)

        self.clear_groups()
        for group in groups:
            self.add_group(group)

        self.clear_pairs()
        for pair in pairs:
            self.add_pair(pair)
