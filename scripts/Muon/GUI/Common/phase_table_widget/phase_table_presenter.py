from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.utilities.algorithm_utils import run_CalMuonDetectorPhases, run_PhaseQuad
from Muon.GUI.Common.observer_pattern import Observer, Observable
from mantid.api import AnalysisDataService, WorkspaceGroup
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
        self.instrument_changed_observer = GenericObserver(self.update_current_phase_tables)

        self.phase_table_calculation_complete_notifier = Observable()
        self.phase_quad_calculation_complete_nofifier = Observable()

        self.update_current_phase_tables()

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

    def handle_calculate_phase_quad_button_clicked(self):
        self.update_model_from_view()

        self.phasequad_calculation_thread = self.create_phase_quad_calculation_thread()

        self.phasequad_calculation_thread.threadWrapperSetUp(self.handle_calculation_started, self.handle_calculation_success,
                                                             self.handle_calculation_error)

        self.phasequad_calculation_thread.start()

    def create_phase_quad_calculation_thread(self):
        self._phasequad_calculation_model = ThreadModelWrapper(self.calculate_phase_quad)
        return thread_model.ThreadModel(self._phasequad_calculation_model)

    def calculate_phase_quad(self):
        parameters = self.get_parameters_for_phase_quad()

        phase_quad = run_PhaseQuad(parameters)

        base_name = parameters['InputWorkspace'].split('_')[0] + '_PhaseQuad_phase_table_'\
            + parameters['PhaseTable'].split('_')[0]
        base_name, group = self.calculate_base_name_and_group(base_name)
        self.add_phase_quad_to_ADS(base_name, group, phase_quad)

    def get_parameters_for_phase_quad(self):
        parameters = {}
        if self.context.phase_context.options_dict['phase_table_for_phase_quad'] == 'Construct':
            parameters['PhaseTable'] = self.calculate_phase_table()
        else:
            parameters['PhaseTable'] = self.context.phase_context.options_dict['phase_table_for_phase_quad']

        parameters['InputWorkspace'] = self.context.phase_context.options_dict['phase_quad_input_workspace']

        return parameters

    def add_phase_quad_to_ADS(self, base_name, group, phase_quad):
        AnalysisDataService.addOrReplace(base_name, phase_quad)
        AnalysisDataService.addToGroup(group, base_name)

        self.context.phase_context.add_phase_quad(base_name)
        self.phase_quad_calculation_complete_nofifier.notify_subscribers()

    def handle_calculation_started(self):
        self.view.setEnabled(False)

    def handle_calculation_error(self, error):
        self.view.setEnabled(True)
        self.view.warning_popup(error)

    def handle_calculation_success(self):
        self.phase_table_calculation_complete_notifier.notify_subscribers()
        self.update_current_phase_tables()
        self.view.setEnabled(True)

    def calculate_phase_table(self):
        parameters = self.create_parameters_for_cal_muon_phase_algorithm()

        detector_table, fitting_information = run_CalMuonDetectorPhases(parameters)

        base_name, group = self.calculate_base_name_and_group(parameters['DetectorTable'])
        self.add_phase_table_to_ADS(base_name, group, detector_table)
        self.add_fitting_info_to_ADS_if_required(base_name, group, fitting_information)

        return parameters['DetectorTable']

    def add_phase_table_to_ADS(self, base_name, group, detector_table):
        AnalysisDataService.addOrReplace(base_name, detector_table)
        AnalysisDataService.addToGroup(group, base_name)
        self.context.phase_context.add_phase_table(base_name)

    def add_fitting_info_to_ADS_if_required(self, base_name, group, fitting_information):
        if not self.view.output_fit_information:
            return

        AnalysisDataService.addOrReplace(base_name + '_fit_information', fitting_information)
        AnalysisDataService.addToGroup(group, base_name + '_fit_information')

    def create_parameters_for_cal_muon_phase_algorithm(self):
        parameters = {}

        parameters['FirstGoodData'] = self.context.phase_context.options_dict['first_good_time']
        parameters['LastGoodData'] = self.context.phase_context.options_dict['last_good_time']

        parameters['InputWorkspace'] = self.context.phase_context.options_dict['input_workspace']

        forward_group = self.context.phase_context.options_dict['forward_group']
        parameters['ForwardSpectra'] = self.context.group_pair_context[forward_group].detectors

        backward_group = self.context.phase_context.options_dict['backward_group']
        parameters['BackwardSpectra'] = self.context.group_pair_context[backward_group].detectors

        parameters['DetectorTable'] = parameters['InputWorkspace'].replace('_raw_data', '') + "_phase_table"

        return parameters

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

    def update_current_groups_list(self):
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

    def update_current_phase_tables(self):
        phase_table_list = self.context.phase_context.get_phase_table_list(self.context.data_context.instrument)
        phase_table_list.append('Construct')

        self.view.set_phase_table_combo_box(phase_table_list)

    def calculate_base_name_and_group(self, table_name):
        run = re.search('[0-9]+', table_name).group()
        base_name = table_name
        group = self.context.data_context._base_run_name(run) + ' PhaseTables'

        if not AnalysisDataService.doesExist(group):
            new_group = WorkspaceGroup()
            AnalysisDataService.addOrReplace(group, new_group)
            AnalysisDataService.addToGroup(self.context.data_context._base_run_name(run), group)

        return base_name, group
