# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import  FindPeaksAutomatic #, PeakMatching
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.simpleapi import AnalysisDataService


class EAAutoTabModel(object):

    def __init__(self,context):
        self.context = None
        self.table_entries = None
        self.update_match_table_notifier = GenericObservable()
        self.update_view_notifier = GenericObservable()
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()


    def handle_peak_algorithms(self,parameters):
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

    def handle_calculation_error(self, error):
        print(error)
        self.calculation_finished_notifier.notify_subscribers()


    def _run_peak_algorithms(self, parameters):
        workspace = AnalysisDataService.Instance().retrieve(parameters["workspace"])
        min_energy = parameters["min_energy"]
        max_energy = parameters["max_energy"]
        threshold = parameters["threshold"]
        print(parameters)
        FindPeaksAutomatic(InputWorkspace = workspace , SpectrumNumber = 3 ,StartXValue = min_energy ,
                           EndXValue = max_energy, AcceptanceThreshold = threshold )
        self.update_view_notifier.notify_subscribers()

    def update_match_table(self, likelihood_table_name , all_matches_table_name):
        likelihood_table = AnalysisDataService.Instance().retrieve(likelihood_table_name)
        all_matches_table = AnalysisDataService.Instance().retrieve(all_matches_table_name)
        if likelihood_table.rowCount() >= 6:
            number_of_records = 6
        else:
            number_of_records = likelihood_table.rowCount()

        likelihood_data = likelihood_table.toDict()
        table_entries = []
        for i in range(number_of_records):
            table_entries.append([])
            for column in likelihood_data:
                table_entries[-1].append(likelihood_data[column][i])
        elements_occurence = {}
        for entry in table_entries:
            elements_occurence[entry[0]] = 0

        all_matches_data = all_matches_table.toDict()
        for i in range(all_matches_table.rowCount()):
            element =all_matches_data["Element"][i]
            if element in elements_occurence:
                elements_occurence[element] = elements_occurence[element] + 1

        for entry in table_entries:
            number_of_occurence = elements_occurence[entry[0]]
            entry.append(number_of_occurence)

        self.table_entries = table_entries
        self.update_match_table_notifier.notify_subscribers()
