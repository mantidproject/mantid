# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name, get_group_data_workspace_name,
                                                         get_pair_asymmetry_name, get_base_data_directory,
                                                         get_group_asymmetry_name,
                                                         get_group_asymmetry_unnorm_name,
                                                         get_deadtime_data_workspace_name)
from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, \
    estimate_group_asymmetry_data, run_pre_processing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list
import Muon.GUI.Common.ADSHandler.workspace_naming as wsName
from Muon.GUI.Common.contexts.muon_group_pair_context import get_default_grouping
from Muon.GUI.Common.contexts.muon_gui_context import PlotMode
from Muon.GUI.Common.contexts.muon_context_ADS_observer import MuonContextADSObserver
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper, WorkspaceGroupDefinition
from mantidqt.utils.observer_pattern import Observable
from Muon.GUI.Common.muon_pair import MuonPair
from typing import List

MUON_ANALYSIS_DEFAULT_X_RANGE = [0.0, 15.0]


class MuonContext(object):
    def __init__(self, muon_data_context=None, muon_gui_context=None,
                 muon_group_context=None, base_directory='Muon Data', muon_phase_context=None,
                 workspace_suffix=' MA', fitting_context=None, frequency_context=None):
        self._data_context = muon_data_context
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context
        self._phase_context = muon_phase_context
        self.fitting_context = fitting_context
        self.base_directory = base_directory
        self.workspace_suffix = workspace_suffix

        self.ads_observer = MuonContextADSObserver(
            self.remove_workspace,
            self.clear_context,
            self.workspace_replaced)

        self.gui_context.update(
            {'DeadTimeSource': 'None',
             'LastGoodDataFromFile': True,
             'selected_group_pair': '',
             'PlotMode': PlotMode.Data})

        self.update_view_from_model_notifier = Observable()
        self.update_plots_notifier = Observable()
        self.deleted_plots_notifier = Observable()

    @property
    def data_context(self):
        return self._data_context

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def group_pair_context(self):
        return self._group_pair_context

    @property
    def phase_context(self):
        return self._phase_context

    @property
    def default_data_plot_range(self):
        return MUON_ANALYSIS_DEFAULT_X_RANGE

    def num_periods(self, run):
        return self._data_context.num_periods(run)

    @property
    def current_runs(self):
        return self._data_context.current_runs

    def calculate_group(self, group, run, rebin=False):
        run_as_string = run_list_to_string(run)
        periods_as_string = run_list_to_string(group.periods)

        # A user requirement is that processing can continue if a period is missing from some
        # of the runs. This filters out periods which are not in a given run.
        periods = [period for period in group.periods if period <= self.num_periods(run)]

        # If not periods match return nothing here. The caller then needs to handle this gracefully.
        if not periods:
            return None, None, None

        name = get_group_data_workspace_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        asym_name = get_group_asymmetry_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        asym_name_unnorm = get_group_asymmetry_unnorm_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        group_workspace = calculate_group_data(self, group, run, rebin, name, periods)
        group_asymmetry, group_asymmetry_unnormalised = estimate_group_asymmetry_data(self, group, run, rebin,
                                                                                      asym_name, asym_name_unnorm, periods)

        return group_workspace, group_asymmetry, group_asymmetry_unnormalised

    def calculate_pair(self, pair: MuonPair, run: List[int], rebin: bool=False):
        try:
            forward_group_workspace_name = self._group_pair_context[pair.forward_group].get_counts_workspace_for_run(run, rebin)
            backward_group_workspace_name = self._group_pair_context[pair.backward_group].get_counts_workspace_for_run(run, rebin)
        except KeyError:
            # A key error here means the requested workspace does not exist so return None
            return None

        run_as_string = run_list_to_string(run)
        output_workspace_name = get_pair_asymmetry_name(self, pair.name, run_as_string, rebin=rebin)
        return calculate_pair_data(pair, forward_group_workspace_name, backward_group_workspace_name, output_workspace_name)

    def show_all_groups(self):
        self.calculate_all_groups()
        for run in self._data_context.current_runs:
            with WorkspaceGroupDefinition():
                for group in self._group_pair_context.groups:
                    run_as_string = run_list_to_string(run)
                    group_name = group.name
                    periods = run_list_to_string(group.periods)

                    directory = get_base_data_directory(self, run_as_string)

                    name = get_group_data_workspace_name(self, group_name, run_as_string, periods, rebin=False)
                    asym_name = get_group_asymmetry_name(self, group_name, run_as_string, periods, rebin=False)
                    asym_name_unnorm = get_group_asymmetry_unnorm_name(self, group_name, run_as_string, periods, rebin=False)

                    self.group_pair_context[group_name].show_raw(run, directory + name, directory + asym_name,
                                                                 asym_name_unnorm)

                    if self._do_rebin():
                        name = get_group_data_workspace_name(self, group_name, run_as_string, periods, rebin=True)
                        asym_name = get_group_asymmetry_name(self, group_name, run_as_string, periods, rebin=True)
                        asym_name_unnorm = get_group_asymmetry_unnorm_name(self, group_name, run_as_string, periods, rebin=True)

                        self.group_pair_context[group_name].show_rebin(run, directory + name, directory + asym_name,
                                                                       asym_name_unnorm)

    def show_all_pairs(self):
        self.calculate_all_pairs()
        for run in self._data_context.current_runs:
            with WorkspaceGroupDefinition():
                for pair_name in self._group_pair_context.pair_names:
                    run_as_string = run_list_to_string(run)
                    name = get_pair_asymmetry_name(
                        self,
                        pair_name,
                        run_as_string,
                        rebin=False)
                    directory = get_base_data_directory(
                        self,
                        run_as_string)

                    self.group_pair_context[
                        pair_name].show_raw(run, directory + name)

                    if self._do_rebin():
                        name = get_pair_asymmetry_name(
                            self,
                            pair_name,
                            run_as_string,
                            rebin=True)
                        self.group_pair_context[
                            pair_name].show_rebin(run, directory + name)

    def calculate_all_pairs(self):
        self._calculate_pairs(rebin=False)
        if(self._do_rebin()):
            self._calculate_pairs(rebin=True)

    def _calculate_pairs(self, rebin):
        for run in self._data_context.current_runs:
            for pair in self._group_pair_context.pairs:
                pair_asymmetry_workspace = self.calculate_pair(
                    pair, run, rebin=rebin)
                if not pair_asymmetry_workspace:
                    continue
                pair.update_asymmetry_workspace(
                     pair_asymmetry_workspace,
                     run,
                     rebin=rebin)

    def calculate_all_groups(self):
        self._calculate_groups(rebin=False)
        if self._do_rebin():
            self._calculate_groups(rebin=True)

    def _calculate_groups(self, rebin):
        for run in self._data_context.current_runs:
            run_pre_processing(context=self, run=run, rebin=rebin)
            for group in self._group_pair_context.groups:
                group_workspace, group_asymmetry, group_asymmetry_unormalised = \
                     self.calculate_group(group, run, rebin=rebin)

                # If this run contains none of the relevant periods for the group no
                # workspace is created.
                if not group_workspace:
                    continue

                self.group_pair_context[group.name].update_workspaces(run, group_workspace, group_asymmetry,
                                                                      group_asymmetry_unormalised, rebin=rebin)

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their
        # default values
        if len(self.data_context.current_runs) > 0:
            self.data_context.update_current_data()

            if not self.group_pair_context.groups:
                minimum_number_of_periods = min([self.num_periods(run) for run in self.current_runs])
                self.group_pair_context.reset_group_and_pairs_to_default(
                    self.data_context.current_workspace,
                    self.data_context.instrument,
                    self.data_context.main_field_direction,
                    minimum_number_of_periods)
        else:
            self.data_context.clear()

    def show_raw_data(self):
        self.ads_observer.observeRename(False)
        for run in self.data_context.current_runs:
            with WorkspaceGroupDefinition():
                run_string = run_list_to_string(run)
                loaded_workspace = \
                    self.data_context._loaded_data.get_data(run=run, instrument=self.data_context.instrument)['workspace'][
                        'OutputWorkspace']
                loaded_workspace_deadtime_table = self.data_context._loaded_data.get_data(
                    run=run, instrument=self.data_context.instrument)['workspace']['DataDeadTimeTable']
                directory = get_base_data_directory(
                    self,
                    run_string)

                deadtime_name = get_deadtime_data_workspace_name(self.data_context.instrument,
                                                                 str(run[0]), workspace_suffix=self.workspace_suffix)
                MuonWorkspaceWrapper(loaded_workspace_deadtime_table).show(directory + deadtime_name)
                self.data_context._loaded_data.get_data(
                    run=run, instrument=self.data_context.instrument)['workspace']['DataDeadTimeTable'] = deadtime_name

                if len(loaded_workspace) > 1:
                    # Multi-period data
                    for i, single_ws in enumerate(loaded_workspace):
                        name = directory + get_raw_data_workspace_name(self.data_context.instrument, run_string,
                                                                       True,
                                                                       period=str(i + 1),
                                                                       workspace_suffix=self.workspace_suffix)
                        single_ws.show(name)
                else:
                    # Single period data
                    name = directory + get_raw_data_workspace_name(self.data_context.instrument, run_string,
                                                                   False,
                                                                   workspace_suffix=self.workspace_suffix)
                    loaded_workspace[0].show(name)

        self.ads_observer.observeRename(True)

    def _do_rebin(self):
        return (self.gui_context['RebinType'] == 'Fixed'
                and 'RebinFixed' in self.gui_context and self.gui_context['RebinFixed']) or \
               (self.gui_context['RebinType'] == 'Variable'
                and 'RebinVariable' in self.gui_context and self.gui_context['RebinVariable'])

    def get_detectors_excluded_from_default_grouping_tables(self):
        groups, _, _ = get_default_grouping(
            self.data_context.current_workspace, self.data_context.instrument,
            self.data_context.main_field_direction)
        detectors_in_group = []
        for group in groups:
            detectors_in_group += group.detectors
        detectors_in_group = set(detectors_in_group)

        return [det for det in range(1, self.data_context.num_detectors) if det not in detectors_in_group]

    # Get the groups/pairs for active WS
    def getGroupedWorkspaceNames(self):
        run_numbers = self.data_context.current_runs
        runs = [
            wsName.get_raw_data_workspace_name(
                self.data_context.instrument,
                run_list_to_string(run_number),
                self.data_context.is_multi_period(),
                period=str(period + 1),
                workspace_suffix=self.workspace_suffix)
            for run_number in run_numbers for period in range(self.data_context.num_periods(run_number))]
        return runs

    def first_good_data(self, run):
        if not self.data_context.get_loaded_data_for_run(run):
            return 0.0

        if self.gui_context['FirstGoodDataFromFile']:
            return self.data_context.get_loaded_data_for_run(run)["FirstGoodData"]
        else:
            if 'FirstGoodData' in self.gui_context:
                return self.gui_context['FirstGoodData']
            else:
                self.gui_context['FirstGoodData'] = self.data_context.get_loaded_data_for_run(
                    run)["FirstGoodData"]
                return self.gui_context['FirstGoodData']

    def last_good_data(self, run):
        if not self.data_context.get_loaded_data_for_run(run):
            return 0.0

        if self.gui_context['LastGoodDataFromFile']:
            return round(max(self.data_context.get_loaded_data_for_run(run)["OutputWorkspace"][0].workspace.dataX(0)),
                         2)
        else:
            if 'LastGoodData' in self.gui_context:
                return self.gui_context['LastGoodData']
            else:
                self.gui_context['LastGoodData'] = round(max(self.data_context.get_loaded_data_for_run(run)
                                                             ["OutputWorkspace"][0].workspace.dataX(0)), 2)
                return self.gui_context['LastGoodData']

    def dead_time_table(self, run):
        if self.gui_context['DeadTimeSource'] == 'FromADS':
            return self.gui_context['DeadTimeTable']
        elif self.gui_context['DeadTimeSource'] == 'FromFile':
            return self.data_context.get_loaded_data_for_run(run)["DataDeadTimeTable"]
        elif self.gui_context['DeadTimeSource'] == 'None':
            return None

    def get_group_and_pair(self, group_and_pair):
        group = []
        pair = []
        if group_and_pair == 'All':
            group = self.group_pair_context.group_names
            pair = self.group_pair_context.pair_names
        else:
            group_pair_list = group_and_pair.replace(' ', '').split(',')
            group = [
                group for group in group_pair_list if group in self.group_pair_context.group_names]
            pair = [
                pair for pair in group_pair_list if pair in self.group_pair_context.pair_names]
        return group, pair

    def get_runs(self, runs):
        run_list = []
        if runs == 'All':
            run_list = self.data_context.current_runs
        else:
            run_list = [run_string_to_list(item)
                        for item in runs.replace(' ', '').split(',')]
            flat_list = []
            for sublist in run_list:
                flat_list += [[run] for run in sublist if len(sublist) > 1]
            run_list += flat_list
            run_list = [
                run for run in run_list if run in self.data_context.current_runs]
        return run_list

    def get_list_of_binned_or_unbinned_workspaces_from_equivalents(
            self, input_list):
        equivalent_list = []

        for item in input_list:
            if 'PhaseQuad' in item:
                equivalent_list.append(item)

            equivalent_group_pair = self.group_pair_context.get_equivalent_group_pair(
                item)
            if equivalent_group_pair:
                equivalent_list.append(equivalent_group_pair)

        return equivalent_list

    def remove_workspace(self, workspace):
        # required as the renameHandler returns a name instead of a workspace.
        if isinstance(workspace, str):
            workspace_name = workspace
        else:
            workspace_name = workspace.name()

        self.data_context.remove_workspace_by_name(workspace_name)
        self.group_pair_context.remove_workspace_by_name(workspace_name)
        self.phase_context.remove_workspace_by_name(workspace_name)
        self.fitting_context.remove_workspace_by_name(workspace_name)
        self.gui_context.remove_workspace_by_name(workspace_name)
        self.update_view_from_model_notifier.notify_subscribers(workspace_name)
        self.deleted_plots_notifier.notify_subscribers(workspace)

    def clear_context(self):
        self.data_context.clear()
        self.group_pair_context.clear()
        self.phase_context.clear()
        self.fitting_context.clear()
        self.update_view_from_model_notifier.notify_subscribers()

    def workspace_replaced(self, workspace):
        self.update_plots_notifier.notify_subscribers(workspace)
