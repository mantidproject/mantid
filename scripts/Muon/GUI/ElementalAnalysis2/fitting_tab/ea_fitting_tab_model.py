from Muon.GUI.Common.utilities.load_utils import flatten_run_list
from mantid.api import IFunction
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import GeneralFittingContext


class EAFittingTabModel(GeneralFittingModel):

    def __init__(self, context: ElementalAnalysisContext, fitting_context: GeneralFittingContext):
        super(EAFittingTabModel, self).__init__(context, fitting_context)
        self._current_spectrum = 0

    @property
    def current_spectrum(self):
        return self._current_spectrum

    @current_spectrum.setter
    def current_spectrum(self, spectrum):
        self._current_spectrum = spectrum

    def get_simultaneous_fit_by_specifiers_to_display_from_context(self) -> list:
        """Returns the simultaneous fit by specifiers to display in the view from the context."""
        runs, detectors = self._get_selected_runs_and_detectors()
        if self.simultaneous_fit_by == "Run":
            return runs
        elif self.simultaneous_fit_by == "Detector":
            return detectors
        return []

    def _get_selected_runs_and_detectors(self):
        selected_runs = []
        selected_detectors = []
        for group_name in self.context.group_context.selected_groups:
            run, detector = self.context.group_context.get_detector_and_run_from_workspace_name(group_name)
            selected_runs.append(run)
            selected_detectors.append(detector)
        return list(set(selected_runs)), list(set(selected_detectors))

    def _get_selected_runs_from_run_list(self) -> list:
        """Extract runs from run list of lists, which is in the format [ [run,...,runs],[runs],...,[runs] ]"""
        current_runs = self.context.data_context.current_runs
        return [str(run) for run in flatten_run_list(current_runs)]

    def get_workspace_names_to_display_from_context(self) -> list:
        """Returns the workspace names to display in the view based on the selected run and group/pair options."""
        runs, groups_and_pairs = self.get_selected_runs_groups_and_pairs()
        workspace_names = self.context.get_workspace_names_for(runs, groups_and_pairs, self.fitting_context.fit_to_raw,
                                                               self.simultaneous_fitting_mode)

        return self._check_data_exists(workspace_names)

    def _get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode(self) -> tuple:
        """Returns the runs, groups and pairs that are currently selected for simultaneous fit mode."""
        runs, Detectors = "All", []

        if self.simultaneous_fit_by == "Run":
            runs = self.simultaneous_fit_by_specifier
        elif self.simultaneous_fit_by == "Detector":
            Detectors = [self.simultaneous_fit_by_specifier]
        return runs, Detectors

    def _get_parameters_for_single_fit(self, dataset_name: str, single_fit_function: IFunction) -> dict:
        """Returns the parameters used for a single fit."""
        params = self._get_common_parameters()
        params["Function"] = single_fit_function
        params["WorkspaceIndex"] = self.current_spectrum
        params["InputWorkspace"] = dataset_name
        params["StartX"] = self.current_start_x
        params["EndX"] = self.current_end_x
        return params

    def _get_parameters_for_simultaneous_fit(self, dataset_names: list, simultaneous_function: IFunction) -> dict:
        """Gets the parameters to use for a simultaneous fit."""
        params = self._get_common_parameters()
        params["Function"] = simultaneous_function
        params["WorkspaceIndex"] = self.current_spectrum
        params["InputWorkspace"] = dataset_names
        params["StartX"] = self.fitting_context.start_xs
        params["EndX"] = self.fitting_context.end_xs
        return params
