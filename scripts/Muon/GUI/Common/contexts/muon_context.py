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
                                                         get_deadtime_data_workspace_name,
                                                         get_pair_phasequad_name,
                                                         add_phasequad_extensions, get_diff_asymmetry_name)
from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, \
    estimate_group_asymmetry_data, run_pre_processing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list
from Muon.GUI.Common.utilities.algorithm_utils import run_PhaseQuad, split_phasequad, rebin_ws, apply_deadtime, calculate_diff_data
from Muon.GUI.Common.muon_base_pair import MuonBasePair
import Muon.GUI.Common.ADSHandler.workspace_naming as wsName
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from Muon.GUI.Common.contexts.muon_group_pair_context import get_default_grouping
from Muon.GUI.Common.contexts.plotting_context import PlotMode
from Muon.GUI.Common.contexts.muon_context_ADS_observer import MuonContextADSObserver
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper, WorkspaceGroupDefinition
from mantidqt.utils.observer_pattern import Observable
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_diff import MuonDiff
from typing import List


class MuonContext(object):

    def __init__(self, muon_data_context=None, muon_gui_context=None, muon_group_context=None, corrections_context=None,
                 base_directory='Muon Data', muon_phase_context=None, workspace_suffix=' MA', fitting_context=None,
                 results_context=None, model_fitting_context=None, plot_panes_context=None, frequency_context=None):
        self._data_context = muon_data_context
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context
        self._corrections_context = corrections_context
        self._phase_context = muon_phase_context
        self.fitting_context = fitting_context
        self.results_context = results_context
        self.model_fitting_context = model_fitting_context
        self.base_directory = base_directory
        self.workspace_suffix = workspace_suffix
        self._plot_panes_context = plot_panes_context
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
    def plot_panes_context(self):
        return self._plot_panes_context

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
    def corrections_context(self):
        return self._corrections_context

    @property
    def phase_context(self):
        return self._phase_context

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

        # If not periods match return nothing here. The caller then needs to
        # handle this gracefully.
        if not periods:
            return None, None, None

        name = get_group_data_workspace_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        asym_name = get_group_asymmetry_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        asym_name_unnorm = get_group_asymmetry_unnorm_name(self, group.name, run_as_string, periods_as_string, rebin=rebin)
        group_workspace = calculate_group_data(self, group, run, rebin, name, periods)
        group_asymmetry, group_asymmetry_unnormalised = estimate_group_asymmetry_data(self, group, run, rebin,
                                                                                      asym_name, asym_name_unnorm, periods)

        return group_workspace, group_asymmetry, group_asymmetry_unnormalised

    def calculate_diff(self, diff: MuonDiff, run: List[int], rebin: bool=False):
        try:
            positive_workspace_name = self._group_pair_context[diff.positive].get_asymmetry_workspace_for_run(run, rebin)
            negative_workspace_name = self._group_pair_context[diff.negative].get_asymmetry_workspace_for_run(run, rebin)
        except KeyError:
            # A key error here means the requested workspace does not exist so return None
            return None
        run_as_string = run_list_to_string(run)
        output_workspace_name = get_diff_asymmetry_name(self, diff.name, run_as_string, rebin=rebin)
        return calculate_diff_data(diff, positive_workspace_name, negative_workspace_name, output_workspace_name)

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

    def show_all_diffs(self):
        self.calculate_all_diffs()
        for run in self._data_context.current_runs:
            with WorkspaceGroupDefinition():
                for diff_name in self._group_pair_context.diff_names:
                    run_as_string = run_list_to_string(run)
                    name = get_diff_asymmetry_name(
                        self,
                        diff_name,
                        run_as_string,
                        rebin=False)
                    directory = get_base_data_directory(
                        self,
                        run_as_string)

                    self.group_pair_context[
                        diff_name].show_raw(run, directory + name)

                    if self._do_rebin():
                        name = get_diff_asymmetry_name(
                            self,
                            diff_name,
                            run_as_string,
                            rebin=True)
                        self.group_pair_context[
                            diff_name].show_rebin(run, directory + name)

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

    def _update_phasequads(self, rebin):
        # lets remove the phasequad pairs
        to_rm = []
        for pair in self._group_pair_context.pairs:
            if not isinstance(
                    pair, MuonPair) and isinstance(
                    pair, MuonBasePair):
                to_rm.append(pair)
        # this is to force a reset of phasequads
        for pair in to_rm:
            self.group_pair_context.remove_pair_from_selected_pairs(pair.name)
        # lets remove the phasequads for now -> later will recalculate
        for pair in self.group_pair_context.phasequads:
            self.group_pair_context.remove_phasequad(pair)

    def _calculate_pairs(self, rebin):
        for run in self._data_context.current_runs:

            self._update_phasequads(rebin)
            # construct the pairs
            for pair in self._group_pair_context.pairs:
                if isinstance(pair, MuonPair):
                    pair_asymmetry_workspace = self.calculate_pair(
                        pair, run, rebin=rebin)
                else:
                    continue

                if not pair_asymmetry_workspace:
                    continue
                pair.update_asymmetry_workspace(
                     pair_asymmetry_workspace,
                     run,
                     rebin=rebin)

    def calculate_all_diffs(self):
        self._calculate_diffs(rebin=False)
        if self._do_rebin():
            self._calculate_diffs(rebin=True)

    def _calculate_diffs(self, rebin):
        for run in self._data_context.current_runs:
            # construct the diffs
            for diff in self._group_pair_context.diffs:
                if isinstance(diff, MuonDiff):
                    diff_asymmetry_workspace = self.calculate_diff(
                        diff, run, rebin=rebin)
                else:
                    continue

                if not diff_asymmetry_workspace:
                    continue
                diff.update_asymmetry_workspace(
                     diff_asymmetry_workspace,
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

    def calculate_phasequads(self, name, phasequad_obj):
        self._calculate_phasequads(name, phasequad_obj, rebin=False)
        if self._do_rebin():
            self._calculate_phasequads(name, phasequad_obj, rebin=True)

    def calculate_phasequad(self, phasequad, run, rebin):
        parameters = {}
        parameters['PhaseTable'] = phasequad.phase_table
        run_string = run_list_to_string(run)

        ws_name = get_pair_phasequad_name(
            self, add_phasequad_extensions(
                phasequad.name), run_string, rebin=rebin)

        parameters['InputWorkspace'] = self._run_deadtime(run_string, ws_name)

        phase_quad = run_PhaseQuad(parameters, ws_name)
        phase_quad = self._run_rebin(phase_quad, rebin)

        workspaces = split_phasequad(phase_quad)
        return workspaces

    def _calculate_phasequads(self, name, phasequad_obj, rebin):
        for run in self._data_context.current_runs:
            if self._data_context.num_periods(run) >1:
                raise ValueError("Cannot support multiple periods")

            ws_list = self.calculate_phasequad(phasequad_obj, run, rebin)
            run_string = run_list_to_string(run)
            directory = get_base_data_directory(self, run_string)
            for ws in ws_list:
                muon_workspace_wrapper = MuonWorkspaceWrapper(directory + ws)
                muon_workspace_wrapper.show()

            phasequad_obj.update_asymmetry_workspaces(
                ws_list,
                run,
                rebin=rebin)

    def _run_deadtime(self, run_string, output):
        name =get_raw_data_workspace_name(self.data_context.instrument,
                                          run_string,
                                          multi_period=False,
                                          workspace_suffix=self.workspace_suffix)
        deadtime_table = self.dead_time_table(run_string)
        if deadtime_table:
            return apply_deadtime(name, output, deadtime_table)
        return name

    def _run_rebin(self, name, rebin):
        if rebin:
            params = "1"
            if self.gui_context['RebinType'] == 'Variable' and self.gui_context["RebinVariable"]:
                params = self.gui_context["RebinVariable"]

            if self.gui_context['RebinType'] == 'Fixed' and self.gui_context["RebinFixed"]:
                ws = retrieve_ws(name)
                x_data = ws.dataX(0)
                original_step = x_data[1] - x_data[0]
                params = float(self.gui_context["RebinFixed"]) * original_step
            return rebin_ws(name,params)
        else:
            return name

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their
        # default values
        if len(self.data_context.current_runs) > 0:
            self.data_context.update_current_data()

            if not self.group_pair_context.groups:
                maximum_number_of_periods = max([self.num_periods(run) for run in self.current_runs])
                self.group_pair_context.reset_group_and_pairs_to_default(
                    self.data_context.current_workspace,
                    self.data_context.instrument,
                    self.data_context.main_field_direction,
                    maximum_number_of_periods)
        else:
            self.data_context.clear()

    def show_raw_data(self):
        self.ads_observer.observeRename(False)
        for run in self.data_context.current_runs:
            with WorkspaceGroupDefinition():
                run_string = run_list_to_string(run)
                loaded_workspace = self.data_context._loaded_data.get_data(run=run, instrument=self.data_context.instrument)['workspace'][
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
                                                                       multi_period=True,
                                                                       period=str(i + 1),
                                                                       workspace_suffix=self.workspace_suffix)
                        single_ws.show(name)
                else:
                    # Single period data
                    name = directory + get_raw_data_workspace_name(self.data_context.instrument, run_string,
                                                                   multi_period=False,
                                                                   workspace_suffix=self.workspace_suffix)
                    loaded_workspace[0].show(name)

        self.ads_observer.observeRename(True)

    def _do_rebin(self):
        return (self.gui_context['RebinType'] == 'Fixed'
                and 'RebinFixed' in self.gui_context and self.gui_context['RebinFixed']) or \
               (self.gui_context['RebinType'] == 'Variable'
                and 'RebinVariable' in self.gui_context and self.gui_context['RebinVariable'])

    def do_double_pulse_fit(self):
        return "DoublePulseEnabled" in self.gui_context and self.gui_context["DoublePulseEnabled"]

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
            if isinstance(run, str):
                run = wsName.get_first_run_from_run_string(run)
            return self.data_context.get_loaded_data_for_run([float(run)])["DataDeadTimeTable"]
        elif self.gui_context['DeadTimeSource'] == 'None':
            return None

    def get_group_and_pair(self, group_and_pair):
        if group_and_pair == 'All':
            group = self.group_pair_context.group_names
            pair = self.group_pair_context.pair_names
            group += self.group_pair_context.get_diffs("group")
            pair += self.group_pair_context.get_diffs("pair")
        else:
            group_pair_list = group_and_pair.replace(' ', '').split(',')
            group = [
                group for group in group_pair_list if group in self.group_pair_context.group_names]
            # add group diffs
            diffs = [diff.name for diff in self.group_pair_context.get_diffs("group")]
            group += [diff for diff in group_pair_list if diff in diffs]
            pair = [
                pair for pair in group_pair_list if pair in self.group_pair_context.pair_names]
            diffs = [diff.name for diff in self.group_pair_context.get_diffs("pair")]
            pair += [diff for diff in group_pair_list if diff in diffs]
        return group, pair

    def get_runs(self, runs):
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

    def get_workspace_names_for(self, runs: str, groups_and_pairs: list, fit_to_raw: bool) -> list:
        """Returns the workspace names of the loaded data for the provided runs and groups/pairs."""
        workspace_names = []
        for run in self.get_runs(runs):
            for group_and_pair in groups_and_pairs:
                workspace_names += self.get_workspace_names_of_data_with_run(run, group_and_pair, fit_to_raw)

        return workspace_names

    def get_workspace_names_of_data_with_run(self, run: int, group_and_pair: str, fit_to_raw: bool):
        """Returns the workspace names of the loaded data with the provided run and group/pair."""
        group, pair = self.get_group_and_pair(group_and_pair)

        group_names = self.group_pair_context.get_group_workspace_names([run], group, not fit_to_raw)
        pair_names = self.group_pair_context.get_pair_workspace_names([run], pair, not fit_to_raw)

        return group_names + pair_names

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
        self.results_context.remove_workspace_by_name(workspace_name)
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
