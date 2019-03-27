# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.calculate_pair_and_group import calculate_group_data, calculate_pair_data, estimate_group_asymmetry_data
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

from Muon.GUI.Common.ADSHandler.workspace_naming import (get_raw_data_workspace_name, get_group_data_workspace_name,
                                                         get_pair_data_workspace_name, get_base_data_directory,
                                                         get_raw_data_directory, get_group_data_directory,
                                                         get_pair_data_directory, get_group_asymmetry_name)


class MuonContext(object):
    base_directory = "Muon Data"

    def __init__(self, muon_data_context=MuonDataContext(), muon_gui_context=MuonGuiContext(), muon_group_context=MuonGroupPairContext()):
        self._data_context = muon_data_context
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context

    @property
    def data_context(self):
        return self._data_context

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def group_pair_context(self):
        return self._group_pair_context

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
