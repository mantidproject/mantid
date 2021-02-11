# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext


FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_X_RANGE = [0.0, 1000.0]
FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_Y_RANGE = [0.0, 1000.0]


class FrequencyDomainAnalysisContext(MuonContext):
    def __init__(self, muon_data_context=None, muon_gui_context=None,
                 muon_group_context=None, fitting_context=None, muon_phase_context=None,plotting_context=None, frequency_context=None, freq_plotting_context=None):
        super().__init__(muon_data_context=muon_data_context, muon_gui_context=muon_gui_context,
                         muon_group_context=muon_group_context, plotting_context=plotting_context,
                         fitting_context=fitting_context, muon_phase_context=muon_phase_context)
        self.workspace_suffix = ' FD'
        self.base_directory = 'Frequency Domain'
        self._freq_plotting_context = freq_plotting_context
        self._freq_plotting_context.set_defaults(FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_X_RANGE, FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_Y_RANGE)
        self._frequency_context = frequency_context

    @property
    def default_fitting_plot_range(self):
        return self._freq_plotting_context.default_xlims    

    def get_workspace_names_for_FFT_analysis(self, use_raw=True):
        groups_and_pairs = ','.join(self.group_pair_context.selected_groups_and_pairs)
        workspace_options = self.get_names_of_time_domain_workspaces_to_fit(
            runs='All', group_and_pair=groups_and_pairs, rebin=not use_raw)
        return workspace_options

    def get_names_of_workspaces_to_fit(self, runs='', group_and_pair='',
                                       phasequad=False, rebin=False, freq="None"):

        return self.get_names_of_frequency_domain_workspaces_to_fit(
            runs=runs, group_and_pair=group_and_pair, frequency_type=freq)

    def get_names_of_frequency_domain_workspaces_to_fit(
            self, runs='', group_and_pair='', frequency_type="None"):
        group, pair = self.get_group_and_pair(group_and_pair)
        run_list = self.get_runs(runs)
        names = self._frequency_context.get_frequency_workspace_names(
            run_list, group, pair, frequency_type)
        return names

    def get_names_of_time_domain_workspaces_to_fit(
            self, runs='', group_and_pair='', rebin=False):
        group, pair = self.get_group_and_pair(group_and_pair)
        run_list = self.get_runs(runs)

        group_names = self.group_pair_context.get_group_workspace_names(
            run_list, group, rebin)
        pair_names = self.group_pair_context.get_pair_workspace_names(
            run_list, pair, rebin)
        return group_names + pair_names

    @property
    def window_title(self):
        return "Frequency Domain Analysis"
