# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name, get_group_data_workspace_name,
                                                         get_pair_data_workspace_name, get_base_data_directory,
                                                         get_raw_data_directory, get_group_data_directory,
                                                         get_pair_data_directory, get_group_asymmetry_name)
from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, \
    estimate_group_asymmetry_data
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.contexts.fitting_context import FittingContext
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list
import Muon.GUI.Common.ADSHandler.workspace_naming as wsName
from Muon.GUI.Common.contexts.muon_group_pair_context import get_default_grouping
from Muon.GUI.Common.contexts.muon_context_ADS_observer import MuonContextADSObserver
from Muon.GUI.Common.observer_pattern import Observable


class MuonContext(object):
    def __init__(self, muon_data_context=MuonDataContext(), muon_gui_context=MuonGuiContext(),
                 muon_group_context=MuonGroupPairContext(), base_directory='Muon Data', muon_phase_context= PhaseTableContext(),
                 workspace_suffix=' MA', fitting_context=FittingContext()):

        self._data_context = muon_data_context
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context
        self._phase_context = muon_phase_context
        self.fitting_context = fitting_context
        self.base_directory = base_directory
        self.workspace_suffix = workspace_suffix
        self.ads_observer = MuonContextADSObserver(self.remove_workspace_by_name)

        self.gui_context.update({'DeadTimeSource': 'None', 'LastGoodDataFromFile': True, 'selected_group_pair': ''})

        self.update_view_from_model_notifier = Observable()

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

    def calculate_group(self, group_name, run, rebin=False):
        group_workspace = calculate_group_data(self, group_name, run, rebin)
        group_asymmetry = estimate_group_asymmetry_data(self, group_name, run, rebin)

        return group_workspace, group_asymmetry

    def calculate_pair(self, pair_name, run, rebin=False):
        return calculate_pair_data(self, pair_name, run, rebin)

    def show_all_groups(self):
        self.calculate_all_groups()
        for run in self._data_context.current_runs:
            for group_name in self._group_pair_context.group_names:
                run_as_string = run_list_to_string(run)
                directory = get_base_data_directory(self, run_as_string) + get_group_data_directory(self, run_as_string)

                name = get_group_data_workspace_name(self, group_name, run_as_string, rebin=False)
                asym_name = get_group_asymmetry_name(self, group_name, run_as_string, rebin=False)

                self.group_pair_context[group_name].show_raw(run, directory + name, directory + asym_name)

                if self._do_rebin():
                    name = get_group_data_workspace_name(self, group_name, run_as_string, rebin=True)
                    asym_name = get_group_asymmetry_name(self, group_name, run_as_string, rebin=True)
                    self.group_pair_context[group_name].show_rebin(run, directory + name, directory + asym_name)

    def show_all_pairs(self):
        self.calculate_all_pairs()
        for run in self._data_context.current_runs:
            for pair_name in self._group_pair_context.pair_names:
                run_as_string = run_list_to_string(run)
                name = get_pair_data_workspace_name(self, pair_name, run_as_string, rebin=False)
                directory = get_base_data_directory(self, run_as_string) + get_pair_data_directory(self, run_as_string)

                self.group_pair_context[pair_name].show_raw(run, directory + name)

                if self._do_rebin():
                    name = get_pair_data_workspace_name(self, pair_name, run_as_string, rebin=True)
                    self.group_pair_context[pair_name].show_rebin(run, directory + name)

    def ensure_all_required_data_loaded(self):
        for run in self._data_context.current_runs:
            if not self.data_context.get_loaded_data_for_run(run):
                return False
        return True

    def calculate_all_pairs(self):
        for run in self._data_context.current_runs:
            for pair_name in self._group_pair_context.pair_names:
                pair_asymmetry_workspace = self.calculate_pair(pair_name, run)
                self.group_pair_context[pair_name].update_asymmetry_workspace(pair_asymmetry_workspace, run)

                if self._do_rebin():
                    pair_asymmetry_workspace = self.calculate_pair(pair_name, run, rebin=True)
                    self.group_pair_context[pair_name].update_asymmetry_workspace(pair_asymmetry_workspace, run, rebin=True)

    def calculate_all_groups(self):
        for run in self._data_context.current_runs:
            for group_name in self._group_pair_context.group_names:
                group_workspace, group_asymmetry = self.calculate_group(group_name, run)
                self.group_pair_context[group_name].update_workspaces(run, group_workspace, group_asymmetry, rebin=False)

                if self._do_rebin():
                    group_workspace, group_asymmetry = self.calculate_group(group_name, run, rebin=True)
                    self.group_pair_context[group_name].update_workspaces(run, group_workspace, group_asymmetry, rebin=True)

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their default values
        if len(self.data_context.current_runs) > 0:
            self.data_context.update_current_data()

            if not self.group_pair_context.groups:
                self.group_pair_context.reset_group_and_pairs_to_default(self.data_context.current_workspace,
                                                                         self.data_context.instrument,
                                                                         self.data_context.main_field_direction)
        else:
            self.data_context.clear()

    def show_raw_data(self):
        for run in self.data_context.current_runs:
            run_string = run_list_to_string(run)
            loaded_workspace = self.data_context._loaded_data.get_data(run=run, instrument=self.data_context.instrument)['workspace'][
                'OutputWorkspace']
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

    def _do_rebin(self):
        return (self.gui_context['RebinType'] == 'Fixed' and
                'RebinFixed' in self.gui_context and self.gui_context['RebinFixed']) or\
               (self.gui_context['RebinType'] == 'Variable' and
                'RebinVariable' in self.gui_context and self.gui_context['RebinVariable'])

    def get_workspace_names_for_FFT_analysis(self, use_raw=True):
        pair_names = list(self.group_pair_context.pair_names)
        group_names = list(self.group_pair_context.group_names)
        run_numbers = self.data_context.current_runs
        workspace_options = []

        for run in run_numbers:
            workspace_options += self.phase_context.get_phase_quad(self.data_context.instrument, run_list_to_string(run))

            for name in pair_names:
                workspace_options.append(
                    wsName.get_pair_data_workspace_name(self,
                                                        str(name),
                                                        run_list_to_string(run), not use_raw))
            for group_name in group_names:
                workspace_options.append(
                    wsName.get_group_asymmetry_name(self, str(group_name), run_list_to_string(run),
                                                    not use_raw))
        return workspace_options

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
            wsName.get_raw_data_workspace_name(self, run_list_to_string(run_number), period=str(period + 1))
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
                self.gui_context['FirstGoodData'] = self.data_context.get_loaded_data_for_run(run)["FirstGoodData"]
                return self.gui_context['FirstGoodData']

    def last_good_data(self, run):
        if not self.data_context.get_loaded_data_for_run(run):
            return 0.0

        if self.gui_context['LastGoodDataFromFile']:
            return round(max(self.data_context.get_loaded_data_for_run(run)["OutputWorkspace"][0].workspace.dataX(0)), 2)
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

    def get_names_of_workspaces_to_fit(self, runs='', group_and_pair='', phasequad=False, rebin=False):
        if group_and_pair == 'All':
            group = self.group_pair_context.group_names
            pair = self.group_pair_context.pair_names
        else:
            group_pair_list = group_and_pair.replace(' ', '').split(',')
            group = [group for group in group_pair_list if group in self.group_pair_context.group_names]
            pair = [pair for pair in group_pair_list if pair in self.group_pair_context.pair_names]

        if runs == 'All':
            run_list = self.data_context.current_runs
        else:
            run_list = [run_string_to_list(item) for item in runs.replace(' ', '').split(',')]
            flat_list = []
            for sublist in run_list:
                flat_list += [[run] for run in sublist if len(sublist) > 1]
            run_list += flat_list
            run_list = [run for run in run_list if run in self.data_context.current_runs]

        group_names = self.group_pair_context.get_group_workspace_names(run_list, group, rebin)
        pair_names = self.group_pair_context.get_pair_workspace_names(run_list, pair, rebin)

        phasequad_names = []
        if phasequad:
            for run in run_list:
                run_string = run_list_to_string(run)
                phasequad_names += self.phase_context.get_phase_quad(self.data_context.instrument, run_string)

        return group_names + pair_names + phasequad_names

    def get_list_of_binned_or_unbinned_workspaces_from_equivalents(self, input_list):
        equivalent_list = []

        for item in input_list:
            if 'PhaseQuad' in item:
                equivalent_list.append(item)

            equivalent_group_pair = self.group_pair_context.get_equivalent_group_pair(item)
            if equivalent_group_pair:
                equivalent_list.append(equivalent_group_pair)

        return equivalent_list

    def remove_workspace_by_name(self, workspace_name):
        self.data_context.remove_workspace_by_name(workspace_name)
        self.group_pair_context.remove_workspace_by_name(workspace_name)
        self.fitting_context.remove_workspace_by_name(workspace_name)
        self.update_view_from_model_notifier.notify_subscribers()
