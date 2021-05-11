# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext


class DataAnalysisContext(MuonContext):
    def __init__(self, muon_data_context=None, muon_gui_context=None,
                 muon_group_context=None, fitting_context=None, muon_phase_context=None, plotting_context=None):
        super().__init__(muon_data_context=muon_data_context, muon_gui_context=muon_gui_context,
                         muon_group_context=muon_group_context, fitting_context=fitting_context,
                         muon_phase_context=muon_phase_context, plotting_context=plotting_context)
        self.workspace_suffix = ' MA'
        self.base_directory = 'Muon Data'

    def get_workspace_names_of_fit_data_with_run(self, run: int, group_and_pair: str) -> list:
        """Returns the workspace names of the data to fit with the provided run and group/pair."""
        return self.get_workspace_names_of_data_with_run(run, group_and_pair)

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
