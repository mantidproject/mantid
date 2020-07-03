# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string


class FrequencyDomainAnalysisContext(MuonContext):
    def __init__(self, muon_data_context=None, muon_gui_context=None,
                 muon_group_context=None, fitting_context=None, muon_phase_context=None, frequency_context=None):
        super().__init__(muon_data_context=muon_data_context, muon_gui_context=muon_gui_context,
        muon_group_context=muon_group_context, fitting_context=fitting_context, muon_phase_context=muon_phase_context)
        self.workspace_suffix = 'FA'
        self.base_directory = 'Frequency Domain'

        self._frequency_context = frequency_context

    @property
    def phase_context(self):
        return self._phase_context

    def get_workspace_names_for_FFT_analysis(self, use_raw=True):
        # workspace_options = self.get_names_of_workspaces_to_fit(
        #     runs='All', group_and_pair='All', phasequad=True, rebin=not use_raw)
        workspace_options = self.get_names_of_time_domain_workspaces_to_fit(runs='All', group_and_pair='All', phasequad=True, rebin=not use_raw)
        return workspace_options

    def get_names_of_workspaces_to_fit(
            self, runs='', group_and_pair='', phasequad=False, rebin=False, freq="None"):
        return self.get_names_of_frequency_domain_workspaces_to_fit(runs=runs, group_and_pair=group_and_pair,
                                                                        phasequad=phasequad, frequency_type=freq)

    def get_names_of_frequency_domain_workspaces_to_fit(
            self, runs='', group_and_pair='', phasequad=False, frequency_type="None"):
        group, pair = self.get_group_and_pair(group_and_pair)
        run_list = self.get_runs(runs)
        names = self._frequency_context.get_frequency_workspace_names(
            run_list, group, pair, phasequad, frequency_type)
        return names

    def get_names_of_time_domain_workspaces_to_fit(
            self, runs='', group_and_pair='', phasequad=False, rebin=False):
        group, pair = self.get_group_and_pair(group_and_pair)
        run_list = self.get_runs(runs)

        group_names = self.group_pair_context.get_group_workspace_names(
            run_list, group, rebin)
        pair_names = self.group_pair_context.get_pair_workspace_names(
            run_list, pair, rebin)

        phasequad_names = []
        if phasequad:
            for run in run_list:
                run_string = run_list_to_string(run)
                phasequad_names += self.phase_context.get_phase_quad(
                    self.data_context.instrument, run_string)
        return group_names + pair_names + phasequad_names

    @property
    def window_title(self):
        return "Frequency Domain Analysis"
