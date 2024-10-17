# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    get_run_numbers_as_string_from_workspace_name,
    get_base_data_directory,
    get_phase_table_workspace_name,
)
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import run_CalMuonDetectorPhases
from mantid import AlgorithmManager


class PhaseTableModel:
    def __init__(self, context):
        self._context = context
        self._current_alg = None
        self._phasequad = None
        self._new_table_name = ""
        self._calculation_thread = None
        self._phasequad_calculation_thread = None

    @property
    def context(self):
        return self._context

    @property
    def phase_context(self):
        return self._context.phase_context

    @property
    def group_pair_context(self):
        return self._context.group_pair_context

    @property
    def phasequad(self):
        return self._phasequad

    @phasequad.setter
    def phasequad(self, value):
        self._phasequad = value

    @property
    def new_table_name(self):
        return self._new_table_name

    @new_table_name.setter
    def new_table_name(self, value):
        self._new_table_name = value

    @property
    def calculation_thread(self):
        return self._calculation_thread

    @calculation_thread.setter
    def calculation_thread(self, value):
        self._calculation_thread = value

    @property
    def phasequad_calculation_thread(self):
        return self._phasequad_calculation_thread

    @phasequad_calculation_thread.setter
    def phasequad_calculation_thread(self, value):
        self._phasequad_calculation_thread = value

    @property
    def group_pair_names(self):
        return self._context.group_pair_context.group_names

    @property
    def group_pairs(self):
        return self._context.group_pair_context.pairs

    @property
    def group_phasequads(self):
        return self._context.group_pair_context.phasequads

    @property
    def instrument(self):
        return self._context.data_context.instrument

    @instrument.setter
    def instrument(self, value):
        self._context.data_context.instrument = value

    def cancel_current_alg(self):
        if self._current_alg is not None:
            self._current_alg.cancel()

    def clear_current_alg(self):
        self._current_alg = None

    def get_grouped_workspace_names(self):
        return self._context.getGroupedWorkspaceNames()

    def add_phase_table_to_ads(self, base_name):
        run = get_run_numbers_as_string_from_workspace_name(base_name, self.instrument)
        directory = get_base_data_directory(self._context, run)
        muon_workspace_wrapper = MuonWorkspaceWrapper(directory + base_name)
        muon_workspace_wrapper.show()
        self.phase_context.add_phase_table(muon_workspace_wrapper)

    @staticmethod
    def add_fitting_info_to_ads(fit_workspace_name):
        muon_workspace_wrapper = MuonWorkspaceWrapper(fit_workspace_name)
        muon_workspace_wrapper.show()

    def create_parameters_for_cal_muon_phase_algorithm(self):
        parameters = dict()
        parameters["FirstGoodData"] = self.phase_context.options_dict["first_good_time"]
        parameters["LastGoodData"] = self.phase_context.options_dict["last_good_time"]
        parameters["InputWorkspace"] = self.phase_context.options_dict["input_workspace"]
        forward_group = self.phase_context.options_dict["forward_group"]
        parameters["ForwardSpectra"] = self.group_pair_context[forward_group].detectors
        backward_group = self.phase_context.options_dict["backward_group"]
        parameters["BackwardSpectra"] = self.group_pair_context[backward_group].detectors
        parameters["DetectorTable"] = get_phase_table_workspace_name(
            parameters["InputWorkspace"], forward_group, backward_group, new_name=self.new_table_name
        )
        return parameters

    def CalMuonDetectorPhases_wrapper(self, parameters, fitting_workspace_name):
        self.current_alg = AlgorithmManager.create("CalMuonDetectorPhases")
        detector_table, fitting_information = run_CalMuonDetectorPhases(parameters, self.current_alg, fitting_workspace_name)
        self.current_alg = None

        return detector_table, fitting_information

    def remove_phasequad(self, phasequad):
        self.group_pair_context.remove_phasequad(phasequad)
