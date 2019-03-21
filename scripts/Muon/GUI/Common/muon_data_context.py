# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.utilities.load_utils as load_utils
import Muon.GUI.Common.utilities.xml_utils as xml_utils
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

from Muon.GUI.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name, get_group_data_workspace_name,
                                                         get_pair_data_workspace_name, get_base_data_directory,
                                                         get_raw_data_directory, get_group_data_directory,
                                                         get_pair_data_directory, get_group_asymmetry_name)

from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, estimate_group_asymmetry_data
from Muon.GUI.Common.utilities.muon_file_utils import allowed_instruments

from collections import OrderedDict

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
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = load_data
        self._gui_variables = {'SummedPeriods': [1], 'SubtractedPeriods': []}
        self._current_data = {"workspace": load_utils.empty_loaded_data()}  # self.get_result(False)

        self._current_runs = []
        self._main_field_direction = ''

        self._instrument = ConfigService.getInstrument().name() if ConfigService.getInstrument().name()\
            in allowed_instruments else 'EMU'

        self.instrumentNotifier = MuonDataContext.InstrumentNotifier(self)
        self.message_notifier = MuonDataContext.MessageNotifier(self)
        self.gui_variables_notifier = MuonDataContext.GuiVariablesNotifier(self)

    def is_data_loaded(self):
        return self._loaded_data.num_items() > 0

    def is_multi_period(self):
        return len(self.current_data["OutputWorkspace"]) > 1

    @property
    def current_data(self):
        return self._current_data["workspace"]

    @property
    def instrument(self):
        return self._instrument

    @instrument.setter
    def instrument(self, value):
        ConfigService['default.instrument'] = value
        self._instrument = value
        self.main_field_direction = ''
        self.set_groups_and_pairs_to_default()
        self.instrumentNotifier.notify_subscribers(self._instrument)

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

    @property
    def gui_variables(self):
        return self._gui_variables

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

    @property
    def first_good_data(self):
        if self.gui_variables['FirstGoodDataFromFile']:
            return self.loaded_data(self.current_runs[-1])["FirstGoodData"]
        else:
            return self.gui_variables['FirstGoodData']

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        if self.check_group_contains_valid_detectors(group):
            self._groups[group.name] = group
        else:
            raise ValueError('Invalid detectors in group {}'.format(group.name))

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._pairs[pair.name] = pair

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their default values
        if len(self.current_runs) > 0:
            self._current_data = self._loaded_data.get_data(run=self.current_runs[0], instrument=self.instrument)
            self.main_field_direction = self.current_data['MainFieldDirection']
            if not self.groups:
                self.set_groups_and_pairs_to_default()
        else:
            self._current_data = {"workspace": load_utils.empty_loaded_data()}

    def loaded_data(self, run):
        loaded_dict = self._loaded_data.get_data(run=run, instrument=self.instrument)
        if loaded_dict:
            return self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']
        else:
            return None

    @property
    def loaded_workspace(self):
        return self.current_data["OutputWorkspace"][0].workspace

    def loaded_workspace_as_group(self, run):
        if self.is_multi_period():
            workspace_group = WorkspaceGroup()
            for workspace_wrapper in self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace']:
                workspace_group.addWorkspace(workspace_wrapper.workspace)
            return workspace_group
        else:
            return self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace'][0].workspace

    def period_string(self, run=None):
        summed_periods = self.gui_variables["SummedPeriods"] if 'SummedPeriods' in self.gui_variables else [1]
        subtracted_periods = self.gui_variables["SubtractedPeriods"] if 'SubtractedPeriods' in self.gui_variables else []
        if subtracted_periods:
            return '+'.join([str(period) for period in summed_periods]) + '-' + '-'.join([str(period) for period in subtracted_periods])
        else:
            return '+'.join([str(period) for period in summed_periods])

    @property
    def num_detectors(self):
        try:
            n_det = self.loaded_workspace.detectorInfo().size()
        except AttributeError:
            # default to 1
            n_det = 1
        return n_det

    def num_periods(self, run):
        return len(self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace'])

    @property
    def main_field_direction(self):
        return self._main_field_direction

    @main_field_direction.setter
    def main_field_direction(self, value):
            if value and value != self._main_field_direction and self._main_field_direction:
                self.message_notifier.notify_subscribers('MainFieldDirection has changed between'
                                                         ' data sets, click default to reset grouping if required')
            self._main_field_direction = value

    @property
    def dead_time_table(self):
        return self.current_data["DeadTimeTable"]

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

    def show_raw_data(self):
        for run in self.current_runs:
            run_string = run_list_to_string(run)
            loaded_workspace = self._loaded_data.get_data(run=run, instrument=self.instrument)['workspace']['OutputWorkspace']
            directory = get_base_data_directory(self, run_string) + get_raw_data_directory(self, run_string)

            if len(loaded_workspace) > 1:
                # Multi-period data
                for i, single_ws in enumerate(loaded_workspace):
                    name = directory + get_raw_data_workspace_name(self, run_string, period=str(i + 1))
                    single_ws.show(name)
            else:
                # Single period data
                name = directory + get_raw_data_workspace_name(self, run_string)
                loaded_workspace[0].show(name)

    def show_all_groups(self):
        for group_name in self._groups.keys():
            self.show_group_data(group_name)

        if self.do_rebin():
            for group_name in self._groups.keys():
                self.show_group_data(group_name, rebin=True)

    def show_group_data(self, group_name, show=True, rebin=False):
        for run in self.current_runs:
            run_as_string = run_list_to_string(run)
            group_workspace = calculate_group_data(self, group_name, run, rebin)
            group_asymmetry = estimate_group_asymmetry_data(self, group_name, run, rebin)
            directory = get_base_data_directory(self, run_as_string) + get_group_data_directory(self, run_as_string)

            name = get_group_data_workspace_name(self, group_name, run_as_string, rebin)
            asym_name = get_group_asymmetry_name(self, group_name, run_as_string, rebin)

            if not rebin:
                self._groups[group_name]._workspace[str(run)] = MuonWorkspaceWrapper(group_workspace)
                self._groups[group_name]._asymmetry_estimate[str(run)] = MuonWorkspaceWrapper(group_asymmetry)
                if show:
                    self._groups[group_name].workspace[str(run)].show(directory + name)
                    self._groups[group_name]._asymmetry_estimate[str(run)].show(directory + asym_name)
            else:
                self._groups[group_name]._workspace_rebin[str(run)] = MuonWorkspaceWrapper(group_workspace)
                self._groups[group_name]._asymmetry_estimate_rebin[str(run)] = MuonWorkspaceWrapper(group_asymmetry)
                if show:
                    self._groups[group_name]._workspace_rebin[str(run)].show(directory + name)
                    self._groups[group_name]._asymmetry_estimate_rebin[str(run)].show(directory + asym_name)

    def show_all_pairs(self):
        for pair_name in self._pairs.keys():
            self.show_pair_data(pair_name)

        if self.do_rebin():
            for pair_name in self._pairs.keys():
                self.show_pair_data(pair_name, rebin=True)

    def show_pair_data(self, pair_name, show=True, rebin=False):
        for run in self.current_runs:
            run_as_string = run_list_to_string(run)
            name = get_pair_data_workspace_name(self, pair_name, run_as_string, rebin)
            directory = get_base_data_directory(self, run_as_string) + get_pair_data_directory(self, run_as_string)
            pair_workspace = calculate_pair_data(self, pair_name, run, rebin)

            if not rebin:
                self._pairs[pair_name].workspace[str(run)] = MuonWorkspaceWrapper(pair_workspace)
                if show:
                    self._pairs[pair_name].workspace[str(run)].show(directory + name)
            else:
                self._pairs[pair_name].workspace_rebin[str(run)] = MuonWorkspaceWrapper(pair_workspace)
                if show:
                    self._pairs[pair_name].workspace_rebin[str(run)].show(directory + name)

    def calculate_all_groups(self):
        for group_name in self._groups.keys():
            calculate_group_data(self, group_name)

    def set_groups_and_pairs_to_default(self):
        groups, pairs = get_default_grouping(self.loaded_workspace, self.instrument, self.main_field_direction)

        self.clear_groups()
        for group in groups:
            self.add_group(group)

        self.clear_pairs()
        for pair in pairs:
            self.add_pair(pair)

    def check_group_contains_valid_detectors(self, group):
        if max(group.detectors) > self.num_detectors or min(group.detectors) < 1:
            return False
        else:
            return True

    def add_or_replace_gui_variables(self, **kwargs):
        if all([key in self._gui_variables and self._gui_variables[key] == kwargs[key] for key in kwargs.keys()]) and kwargs:
            return
        self._gui_variables.update(kwargs)
        self.gui_variables_notifier.notify_subscribers()

    def do_rebin(self):
        return (self.gui_variables['RebinType'] == 'Fixed' and
                'RebinFixed' in self.gui_variables and self.gui_variables['RebinFixed']) or\
               (self.gui_variables['RebinType'] == 'Variable' and
                'RebinVariable' in self.gui_variables and self.gui_variables['RebinVariable'])

    def check_run_list_are_all_same_field(self, run_list):
        if not run_list:
            return True

        first_field = self._loaded_data.get_main_field_direction(run=run_list[0], instrument=self.instrument)
        return all(first_field==self._loaded_data.get_main_field_direction(run=run, instrument=self.instrument)
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

    class GuiVariablesNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
