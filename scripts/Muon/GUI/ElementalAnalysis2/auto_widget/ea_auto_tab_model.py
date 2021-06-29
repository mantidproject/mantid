# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FindPeaksAutomatic, PeakMatching, GroupWorkspaces
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, remove_ws
from Muon.GUI.Common import thread_model, message_box
from mantidqt.utils.observer_pattern import GenericObservable
from queue import Queue
import copy

"""
Peak width for detectors are constant between runs are defined below for each detector,
data was extracted from run 2749
"""
PEAK_WIDTH = {"Detector 1": [0.5, 1, 2.5],
              "Detector 2": [0.5, 1, 3],
              "Detector 3": [0.1, 0.5, 1.5],
              "Detector 4": [0.1, 0.7, 1.5]}

NUMBER_OF_ELEMENTS_DISPLAYED = 3

# Workspace suffixes
REFITTED_PEAKS_WS_SUFFIX = "_refitted_peaks"
PEAKS_WS_SUFFIX = "_peaks"
ERRORS_WS_SUFFIX = "_with_errors"
MATCH_TABLE_WS_SUFFIXES = ["_all_matches", "_primary_matches", "_secondary_matches", "_all_matches_sorted_by_energy",
                           "_likelihood"]
MATCH_GROUP_WS_SUFFIX = "_matches"


def find_peak_algorithm(workspace, spectrum_number, min_energy, max_energy, threshold, min_width,
                        estimate_width, max_width):
    FindPeaksAutomatic(InputWorkspace=retrieve_ws(workspace), SpectrumNumber=spectrum_number, StartXValue=min_energy,
                       EndXValue=max_energy, AcceptanceThreshold=threshold,
                       PeakPropertiesTableName=workspace + PEAKS_WS_SUFFIX,
                       RefitPeakPropertiesTableName=workspace + REFITTED_PEAKS_WS_SUFFIX, MinPeakSigma=min_width,
                       EstimatePeakSigma=estimate_width, MaxPeakSigma=max_width)


def peak_matching_algorithm(workspace, match_table_names):
    PeakMatching(PeakTable=workspace + PEAKS_WS_SUFFIX, AllPeaks=match_table_names[0],
                 PrimaryPeaks=match_table_names[1], SecondaryPeaks=match_table_names[2],
                 SortedByEnergy=match_table_names[3], ElementLikelihood=match_table_names[4])


class EAAutoTabModel(object):

    def __init__(self, context):
        self.context = context
        self.table_entries = Queue()
        self.warnings = Queue()
        self.update_match_table_notifier = GenericObservable()
        self.update_view_notifier = GenericObservable()
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()
        self.current_peak_table_info = {"workspace": None, "number_of_peaks": None}

    def handle_warnings(self):
        while not self.warnings.empty():
            warning = self.warnings.get()
            message_box.warning("WARNING: " + str(warning), None)

    def split_run_and_detector(self, workspace_name):
        run_and_detector = workspace_name.split(";")
        if len(run_and_detector) == 1:
            return None, None
        run, detector = run_and_detector
        return run.strip(), detector.strip()

    def handle_peak_algorithms(self, parameters):
        self.peak_algo_model = ThreadModelWrapper(lambda: self._run_peak_algorithms(parameters))
        self.peak_algo_thread = thread_model.ThreadModel(self.peak_algo_model)
        self.peak_algo_thread.threadWrapperSetUp(self.handle_calculation_started,
                                                 self.calculation_success,
                                                 self.handle_calculation_error)
        self.peak_algo_thread.start()

    def handle_calculation_started(self):
        self.calculation_started_notifier.notify_subscribers()

    def calculation_success(self):
        self.calculation_finished_notifier.notify_subscribers()
        self.handle_warnings()
        self.update_view_notifier.notify_subscribers()
        self.update_match_table_notifier.notify_subscribers()

    def handle_calculation_error(self, error):
        message_box.warning("ERROR: " + str(error), None)
        self.handle_warnings()
        self.calculation_finished_notifier.notify_subscribers()
        self.update_view_notifier.notify_subscribers()
        self.update_match_table_notifier.notify_subscribers()

    def _run_peak_algorithms(self, parameters):
        workspace = parameters["workspace"]
        run, detector = self.split_run_and_detector(workspace)
        if run is None or detector is None:
            group_workspace = retrieve_ws(workspace)
            for group in self.context.group_context.groups:
                if group.run_number == workspace:
                    workspace_name = group.get_counts_workspace_for_run(workspace, rebin=False)
                    tmp_parameters = copy.deepcopy(parameters)
                    tmp_parameters["workspace"] = workspace_name
                    if not self._run_find_peak_algorithm(tmp_parameters, group_workspace, True):
                        self._run_peak_matching_algorithm(workspace_name, group_workspace)
        else:
            group_workspace = retrieve_ws(run)
            self._run_find_peak_algorithm(parameters, group_workspace)
            self._run_peak_matching_algorithm(workspace, group_workspace)

    def _run_find_peak_algorithm(self, parameters, group, delay_errors=False):
        """
        Run FindPeaksAutomatic algorithm and then adds output workspace to run group workspace, it also adds tables to
        run group workspace if they are not empty or it is deleted.
        """

        workspace = parameters["workspace"]
        min_energy = parameters["min_energy"]
        max_energy = parameters["max_energy"]
        threshold = parameters["threshold"]
        run, detector = self.split_run_and_detector(workspace)
        if parameters["default_width"]:
            min_width = PEAK_WIDTH[detector][0]
            max_width = PEAK_WIDTH[detector][2]
            estimate_width = PEAK_WIDTH[detector][1]
        else:
            min_width = parameters["min_width"]
            max_width = parameters["max_width"]
            estimate_width = parameters["estimate_width"]

        find_peak_algorithm(workspace, 3, min_energy, max_energy, threshold, min_width, estimate_width, max_width)

        return self._handle_find_peak_algorithm_outputs(group, workspace, delay_errors)

    def _run_peak_matching_algorithm(self, workspace, group_workspace):
        """
        Run PeakMatching algorithm and adds resulting table workspaces into a matches group workspace, matches group
        workspace is then added is then added to run group workspace.
        """
        match_table_names = [workspace + suffix for suffix in MATCH_TABLE_WS_SUFFIXES]

        peak_matching_algorithm(workspace, match_table_names)

        GroupWorkspaces(InputWorkspaces=match_table_names, OutputWorkspace=workspace + MATCH_GROUP_WS_SUFFIX)

        group_workspace.add(workspace + MATCH_GROUP_WS_SUFFIX)
        self.context.group_context[workspace].update_matches_table(group_workspace.name(),
                                                                   workspace + MATCH_GROUP_WS_SUFFIX)

        self.update_match_table(workspace + MATCH_TABLE_WS_SUFFIXES[-1], workspace)

    def update_match_table(self, likelihood_table_name, workspace_name):
        likelihood_table = retrieve_ws(likelihood_table_name)
        entry = list(self.split_run_and_detector(workspace_name))

        likelihood_data = likelihood_table.toDict()
        if likelihood_table.rowCount() > NUMBER_OF_ELEMENTS_DISPLAYED:
            elements_list = likelihood_data["Element"][:NUMBER_OF_ELEMENTS_DISPLAYED]
        else:
            elements_list = likelihood_data["Element"]
        elements = " , ".join(elements_list)
        entry.append(elements)
        self.table_entries.put(entry)

    def _handle_find_peak_algorithm_outputs(self, group_workspace, workspace, delay_errors):
        ignore_peak_matching = False
        remove_ws(workspace + ERRORS_WS_SUFFIX)
        remove_ws(workspace + REFITTED_PEAKS_WS_SUFFIX)

        peak_table = retrieve_ws(workspace + PEAKS_WS_SUFFIX)
        number_of_peaks = peak_table.rowCount()
        self.current_peak_table_info["workspace"] = workspace
        self.current_peak_table_info["number_of_peaks"] = number_of_peaks
        if number_of_peaks != 0:
            group_workspace.add(workspace + PEAKS_WS_SUFFIX)
            self.context.group_context[workspace].update_peak_table(group_workspace.name(), workspace + PEAKS_WS_SUFFIX)
        else:
            peak_table.delete()
            if delay_errors:
                self.warnings.put(f"No peaks found in {workspace} try reducing acceptance threshold")
                ignore_peak_matching = True
            else:
                raise RuntimeError(f"No peaks found in {workspace} try reducing acceptance threshold")
        return ignore_peak_matching
