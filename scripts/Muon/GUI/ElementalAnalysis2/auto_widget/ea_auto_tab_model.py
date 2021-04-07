# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FindPeaksAutomatic, PeakMatching, GroupWorkspaces
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from Muon.GUI.Common import thread_model, message_box
from mantidqt.utils.observer_pattern import GenericObservable
from queue import Queue
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import check_if_group_is_valid

# Peak width for detectors are defined below for each detector data was extracted from run 2749
PEAK_WIDTH = {"Detector 1": [0.5, 1, 2.5],
              "Detector 2": [0.5, 1, 3],
              "Detector 3": [0.1, 0.5, 1.5],
              "Detector 4": [0.1, 0.7, 1.5]}


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
            return None
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
        run_and_detector = self.split_run_and_detector(workspace)
        if run_and_detector is None:
            run = workspace
            group = retrieve_ws(run)
            for workspace_name in group.getNames():
                if check_if_group_is_valid(workspace_name):
                    parameters["workspace"] = workspace_name
                    if not self.run_find_peak_algorithm(parameters, group, True):
                        self.run_peak_matchiing_algorithm(workspace_name, group)
        else:
            run, detector = run_and_detector
            group = retrieve_ws(run)
            self.run_find_peak_algorithm(parameters, group)
            self.run_peak_matchiing_algorithm(workspace, group)

    def run_find_peak_algorithm(self, parameters, group, delay_errors=False):
        # Run FindPeaksAutomatic algorithm
        ignore_peak_matching = False
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

        FindPeaksAutomatic(InputWorkspace=retrieve_ws(workspace), SpectrumNumber=3, StartXValue=min_energy,
                           EndXValue=max_energy, AcceptanceThreshold=threshold,
                           PeakPropertiesTableName=workspace + "_peaks",
                           RefitPeakPropertiesTableName=workspace + "_refitted_peaks", MinPeakSigma=min_width,
                           EstimatePeakSigma=estimate_width, MaxPeakSigma=max_width)

        group.add(workspace + "_with_errors")

        refit_peak_table = retrieve_ws(workspace + "_refitted_peaks")
        if refit_peak_table.rowCount() != 0:
            group.add(workspace + "_refitted_peaks")
        else:
            refit_peak_table.delete()

        peak_table = retrieve_ws(workspace + "_peaks")
        number_of_peaks = peak_table.rowCount()
        self.current_peak_table_info["workspace"] = workspace
        self.current_peak_table_info["number_of_peaks"] = number_of_peaks
        if number_of_peaks != 0:
            group.add(workspace + "_peaks")
        else:
            peak_table.delete()
            if delay_errors:
                self.warnings.put(f"No peaks found in {workspace} try reducing acceptance threshold")
                ignore_peak_matching = True
            else:
                raise RuntimeError(f"No peaks found in {workspace} try reducing acceptance threshold")
        return ignore_peak_matching

    def run_peak_matchiing_algorithm(self, workspace, group):
        # Run PeakMatching algorithm
        match_table_names = [workspace + "_all_matches", workspace + "_primary_matches",
                             workspace + "_secondary_matches",
                             workspace + "_all_matches_sorted_by_energy", workspace + "_likelihood"]

        PeakMatching(PeakTable=workspace + "_peaks", AllPeaks=match_table_names[0],
                     PrimaryPeaks=match_table_names[1], SecondaryPeaks=match_table_names[2],
                     SortedByEnergy=match_table_names[3], ElementLikelihood=match_table_names[4])

        GroupWorkspaces(InputWorkspaces=match_table_names, OutputWorkspace=workspace + "_matches")

        group.add(workspace + "_matches")
        self.update_match_table(workspace + "_likelihood", workspace)

    def update_match_table(self, likelihood_table_name, workspace_name):
        likelihood_table = retrieve_ws(likelihood_table_name)
        entry = list(self.split_run_and_detector(workspace_name))

        likelihood_data = likelihood_table.toDict()
        if likelihood_table.rowCount() > 3:
            elements_list = likelihood_data["Element"][:3]
        else:
            elements_list = likelihood_data["Element"]
        elements = " , ".join(elements_list)
        entry.append(elements)
        self.table_entries.put(entry)
