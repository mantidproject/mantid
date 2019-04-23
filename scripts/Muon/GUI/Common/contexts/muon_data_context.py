# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.utilities.load_utils as load_utils
import Muon.GUI.Common.utilities.xml_utils as xml_utils
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

from Muon.GUI.Common.utilities.muon_file_utils import allowed_instruments

from mantid.api import WorkspaceGroup
from mantid.kernel import ConfigServiceImpl, ConfigService
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
        self._loaded_data = load_data
        self._current_data = {"workspace": load_utils.empty_loaded_data(), 'run': []}  # self.get_result(False)

        self._current_runs = []
        self._main_field_direction = ''

        self._instrument = ConfigService.getInstrument().name() if ConfigService.getInstrument().name()\
            in allowed_instruments else 'EMU'

        self.instrumentNotifier = MuonDataContext.InstrumentNotifier(self)
        self.message_notifier = MuonDataContext.MessageNotifier(self)

    def is_data_loaded(self):
        return self._loaded_data.num_items() > 0

    def is_multi_period(self):
        return len(self.current_data["OutputWorkspace"]) > 1

    @property
    def instrument(self):
        return self._instrument

    @instrument.setter
    def instrument(self, value):
        if value != self.instrument:
            ConfigService['default.instrument'] = value
            self._instrument = value
            self._main_field_direction = ''
            self.instrumentNotifier.notify_subscribers(self._instrument)

    @property
    def current_runs(self):
        return self._current_runs

    @current_runs.setter
    def current_runs(self, value):
        if not self.check_run_list_are_all_same_field(value):
            self.message_notifier.notify_subscribers(self.create_multiple_field_directions_error_message(value))
        self._current_runs = value

    @property
    def current_filenames(self):
        current_filenames = []
        for run in self.current_runs:
            if self._loaded_data.get_data(run=run, instrument=self.instrument):
                current_filenames.append(self._loaded_data.get_data(run=run, instrument=self.instrument)['filename'])
        return current_filenames

    @property
    def current_workspaces(self):
        current_workspaces = []
        for run in self.current_runs:
            current_workspaces.append(self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace'])
        return current_workspaces

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their default values
        self._current_data = self._loaded_data.get_data(run=self.current_runs[0], instrument=self.instrument)

        if self.current_data['MainFieldDirection'] and self.current_data['MainFieldDirection'] != self._main_field_direction\
                and self._main_field_direction:
            self.message_notifier.notify_subscribers('MainFieldDirection has changed between'
                                                     ' data sets, click default to reset grouping if required')
        self._main_field_direction = self.current_data['MainFieldDirection']

    @property
    def current_data(self):
        return self._current_data["workspace"]

    @property
    def current_workspace(self):
        return self.current_data["OutputWorkspace"][0].workspace

    @property
    def current_run(self):
        return self._current_data["run"]

    def get_loaded_data_for_run(self, run):
        loaded_dict = self._loaded_data.get_data(run=run, instrument=self.instrument)
        if loaded_dict:
            return self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']
        else:
            return None

    def loaded_workspace_as_group(self, run):
        if self.is_multi_period():
            workspace_group = WorkspaceGroup()
            for workspace_wrapper in self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace']:
                workspace_group.addWorkspace(workspace_wrapper.workspace)
            return workspace_group
        else:
            return self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace'][0].workspace

    @property
    def num_detectors(self):
        try:
            n_det = self.current_workspace.detectorInfo().size()
        except AttributeError:
            # default to 1
            n_det = 1
        return n_det

    @property
    def num_points(self):
        workspace_lengths = [len(self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace'][0]
                             .workspace.readX(0)) for run in self.current_runs]
        if workspace_lengths:
            return max(workspace_lengths)
        else:
            return 1

    def num_periods(self, run):
        return len(self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace'])

    @property
    def main_field_direction(self):
        return self._main_field_direction

    @property
    def dead_time_table(self):
        return self.current_data["DeadTimeTable"]

    def __get_sample_logs(self):
        logs = None
        try:
            logs = self.current_workspace.getRun()
        except Exception:
            print("Cannot find sample logs")
        return logs

    def get_sample_log(self, log_name):
        logs = self.__get_sample_logs()
        try:
            log = logs.getLogData(log_name)
        except Exception:
            log = None
        return log

    # ------------------------------------------------------------------------------------------------------------------
    # Clearing data
    # ------------------------------------------------------------------------------------------------------------------
    def clear(self):
        self._current_data = {"workspace": load_utils.empty_loaded_data(), 'run': []}

    def _base_run_name(self, run=None):
        """ e.g. EMU0001234 """
        if not run:
            run = self.run
        if isinstance(run, int):
            return str(self.instrument) + str(run)
        else:
            return str(self.instrument) + run

    # ------------------------------------------------------------------------------------------------------------------
    # Showing workspaces in the ADS
    # ------------------------------------------------------------------------------------------------------------------
    def check_group_contains_valid_detectors(self, group):
        if max(group.detectors) > self.num_detectors or min(group.detectors) < 1:
            return False
        else:
            return True

    def check_run_list_are_all_same_field(self, run_list):
        if not run_list:
            return True

        first_field = self._loaded_data.get_main_field_direction(run=run_list[0], instrument=self.instrument)
        return all(first_field == self._loaded_data.get_main_field_direction(run=run, instrument=self.instrument)
                   for run in run_list)

    def create_multiple_field_directions_error_message(self, run_list):
        transverse = []
        longitudinal = []
        for run in run_list:
            field_direction = self._loaded_data.get_main_field_direction(run=run, instrument=self.instrument)
            if field_direction.lower() == 'transverse':
                transverse += run
            elif field_direction.lower() == 'longitudinal':
                longitudinal += run
            else:
                return 'Unrecognised field direction {} for run {}'.format(field_direction, run)

        message = 'MainFieldDirection changes within current run set:\n'
        message += 'transverse field runs {}\n'.format(run_list_to_string(transverse))
        message += 'longitudinal field runs {}\n'.format(run_list_to_string(longitudinal))
        return message

    class InstrumentNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)

    class MessageNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
