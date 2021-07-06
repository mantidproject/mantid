# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext


class DataAnalysisContext(MuonContext):
    def __init__(self, muon_data_context=None, muon_gui_context=None, muon_group_context=None, corrections_context=None,
                 fitting_context=None, results_context=None, model_fitting_context=None, muon_phase_context=None,
                 plot_panes_context=None):
        super().__init__(muon_data_context=muon_data_context, muon_gui_context=muon_gui_context,
                         muon_group_context=muon_group_context, corrections_context=corrections_context,
                         fitting_context=fitting_context, results_context=results_context,
                         model_fitting_context=model_fitting_context, muon_phase_context=muon_phase_context,
                         plot_panes_context=plot_panes_context)
        self.workspace_suffix = ' MA'
        self.base_directory = 'Muon Data'

    def get_names_of_workspaces_to_fit(
            self, runs='', group_and_pair='', rebin=False):
        return self.get_names_of_time_domain_workspaces_to_fit(
            runs=runs, group_and_pair=group_and_pair, rebin=rebin)

    def get_names_of_time_domain_workspaces_to_fit(
            self, runs='', group_and_pair='', rebin=False):
        group, pair = self.get_group_and_pair(group_and_pair)

        if runs.find(',') != -1 or runs.find('-') != -1:
            # Can assume to be all as if found then must be using co-add which is all runs
            runs = 'All'
        run_list = self.get_runs(runs)

        group_names = self.group_pair_context.get_group_workspace_names(
            run_list, group, rebin)
        pair_names = self.group_pair_context.get_pair_workspace_names(
            run_list, pair, rebin)

        return group_names + pair_names

    @property
    def default_fitting_plot_range(self):
        return self.default_data_plot_range

    @property
    def default_end_x(self):
        return 15.0

    @property
    def guess_workspace_prefix(self):
        return "__muon_analysis_fitting_guess"

    @property
    def window_title(self):
        return "Muon Analysis"
