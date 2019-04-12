from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.utilities.algorithm_utils import run_CalMuonDetectorPhases
from Muon.GUI.Common.observer_pattern import Observer, Observable
from mantid.api import AnalysisDataService
import re

class GenericObserver(Observer):
    def __init__(self, callback):
        Observer.__init__(self)
        self.callback = callback

    def update(self, observable, arg):
        self.callback()

class PhaseTablePresenter(object):
    def __init__(self, view, context):
        self.view = view
        self.context = context

        self.group_change_observer = GenericObserver(self.update_current_groups_list)
        self.run_change_observer = GenericObserver(self.update_current_run_list)

        self.phase_table_calculation_complete_notifier = Observable()

    def update_view_from_model(self):
        for key, item in self.context.phase_context.options_dict.items():
            setattr(self.view, key, item)

    def update_model_from_view(self):
        for key in self.context.phase_context.options_dict:
            self.context.phase_context.options_dict[key] = getattr(self.view, key, None)

    def handle_calulate_phase_table_clicked(self):
        self.update_model_from_view()

        self.calculation_thread = self.create_calculation_thread()

        self.calculation_thread.threadWrapperSetUp(self.handle_calculation_started, self.handle_calculation_success,
                                                   self.handle_calculation_error)

        self.calculation_thread.start()

    def create_calculation_thread(self):
        self._calculation_model = ThreadModelWrapper(self.calculate_phase_table)
        return thread_model.ThreadModel(self._calculation_model)

    def handle_calculation_started(self):
        self.view.setEnabled(False)

    def handle_calculation_error(self, error):
        self.view.setEnabled(True)
        self.view.warning_popup(error)

    def handle_calculation_success(self):
        self.view.setEnabled(True)

    def calculate_phase_table(self):
        parameters = self.create_parameters_for_cal_muon_phase_algorithm()

        detector_table = run_CalMuonDetectorPhases(parameters)

        self.add_phase_table_to_ADS(parameters, detector_table)

    def add_phase_table_to_ADS(self, parameters, detector_table):
        AnalysisDataService.addOrReplace(parameters['DetectorTable'], detector_table)
        run = re.search('[0-9]+', parameters['DetectorTable']).group()
        AnalysisDataService.addToGroup(self.context.data_context._base_run_name(run), parameters['DetectorTable'])

    def create_parameters_for_cal_muon_phase_algorithm(self):

        parameters = {}

        parameters['FirstGoodData'] = self.context.phase_context.options_dict['first_good_time']
        parameters['LastGoodData'] = self.context.phase_context.options_dict['last_good_time']

        parameters['InputWorkspace'] = self.context.phase_context.options_dict['input_workspace']

        forward_group = self.context.phase_context.options_dict['forward_group']
        parameters['ForwardSpectra'] = self.context.group_pair_context[forward_group].detectors

        backward_group = self.context.phase_context.options_dict['backward_group']
        parameters['BackwardSpectra'] = self.context.group_pair_context[backward_group].detectors

        parameters['DetectorTable'] = parameters['InputWorkspace'] + "_phase_table"

        return parameters

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

    def update_current_groups_list(self):
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()
