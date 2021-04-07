# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.utilities.algorithm_utils import run_CalMuonDetectorPhases
from Muon.GUI.Common.muon_phasequad import MuonPhasequad
from mantidqt.utils.observer_pattern import Observable, GenericObserver, GenericObservable
import re
from Muon.GUI.Common.ADSHandler.workspace_naming import get_phase_table_workspace_name, \
    get_fitting_workspace_name, get_base_data_directory, \
    get_run_number_from_workspace_name, \
    get_run_numbers_as_string_from_workspace_name
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.utilities.run_string_utils import valid_name_regex
import mantid


class PhaseTablePresenter(object):
    def __init__(self, view, context):
        self.view = view
        self.context = context
        self.current_alg = None
        self._phasequad_obj = None

        self.group_change_observer = GenericObserver(self.update_current_groups_list)
        self.run_change_observer = GenericObserver(self.update_current_run_list)
        self.instrument_changed_observer = GenericObserver(self.update_current_phase_tables)

        self.phase_table_calculation_complete_notifier = Observable()
        self.phase_quad_calculation_complete_notifier = Observable()
        self.enable_editing_notifier = Observable()
        self.disable_editing_notifier = Observable()

        self.disable_tab_observer = GenericObserver(self.view.disable_widget)
        self.enable_tab_observer = GenericObserver(self.view.enable_widget)
        self.selected_phasequad_changed_notifier = GenericObservable()

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)
        self.update_current_phase_tables()

        self.view.on_first_good_data_changed(self.handle_first_good_data_changed)
        self.view.on_last_good_data_changed(self.handle_last_good_data_changed)

    def update_view_from_model(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_current_phase_tables()
        for key, item in self.context.phase_context.options_dict.items():
            setattr(self.view, key, item)

    def update_model_from_view(self):
        for key in self.context.phase_context.options_dict:
            self.context.phase_context.options_dict[key] = getattr(self.view, key, None)

    def cancel(self):
        if self.current_alg is not None:
            self.current_alg.cancel()

    def handle_calulate_phase_table_clicked(self):
        self.update_model_from_view()
        self.disable_editing_notifier.notify_subscribers()
        self.calculation_thread = self.create_calculation_thread()

        self.calculation_thread.threadWrapperSetUp(self.handle_phase_table_calculation_started,
                                                   self.handle_calculation_success,
                                                   self.handle_calculation_error)

        self.calculation_thread.start()

    def create_calculation_thread(self):
        self._calculation_model = ThreadModelWrapper(self.calculate_phase_table)
        return thread_model.ThreadModel(self._calculation_model)

    def handle_calculate_phase_quad_button_clicked(self):
        self.update_model_from_view()
        self.handle_add_phasequad_button_clicked()

    def validate_pair_name(self, text):
        if text in self.context.group_pair_context.pairs:
            self.view.warning_popup(
                "Groups and pairs (including phasequads) must have unique names")
            return False
        if not re.match(valid_name_regex, text):
            self.view.warning_popup(
                "Phasequad names should only contain digits, characters and _")
            return False
        return True

    def handle_add_phasequad_button_clicked(self):
        if self.view.number_of_phase_tables < 1:
            self.view.warning_popup("Please generate a phase table first.")
            return

        new_pair_name = self.view.enter_pair_name()
        if new_pair_name is None:
            return

        elif self.validate_pair_name(new_pair_name):
            table = self.context.phase_context.options_dict['phase_table_for_phase_quad']
            self._phasequad_obj = MuonPhasequad(str(new_pair_name), table)

            self.phasequad_calculation_thread = self.create_phase_quad_calculation_thread()

            self.phasequad_calculation_thread.threadWrapperSetUp(self.handle_calculation_started,
                                                                 self.handle_phasequad_calculation_success,
                                                                 self.handle_calculation_error)

            self.phasequad_calculation_thread.start()

    def create_phase_quad_calculation_thread(self):
        self._phasequad_calculation_model = ThreadModelWrapper(self.calculate_phase_quad)
        return thread_model.ThreadModel(self._phasequad_calculation_model)

    def calculate_phase_quad(self):
        self.context.group_pair_context.add_phasequad(self._phasequad_obj)

        self.context.calculate_phasequads(
            self._phasequad_obj.name, self._phasequad_obj)
        self.phase_quad_calculation_complete_notifier.notify_subscribers(
            self._phasequad_obj.Re.name)
        self.phase_quad_calculation_complete_notifier.notify_subscribers(
            self._phasequad_obj.Im.name)

    def handle_phasequad_calculation_success(self):
        self.enable_editing_notifier.notify_subscribers()
        self.view.enable_widget()
        self.view.disable_cancel()

        names = [self._phasequad_obj.Re.name, self._phasequad_obj.Im.name]
        self.current_alg = None
        pair_added = True  # if state == 2 else False

        for name in names:
            if pair_added:
                self.context.group_pair_context.add_pair_to_selected_pairs(
                    name)
            else:
                self.context.group_pair_context.remove_pair_from_selected_pairs(
                    name)

            group_info = {'is_added': pair_added, 'name': name}
            self.selected_phasequad_changed_notifier.notify_subscribers(
                group_info)
        self._phasequad_obj = None

    def handle_calculation_started(self):
        self.disable_editing_notifier.notify_subscribers()
        self.view.enable_phasequad_cancel()

    def handle_phase_table_calculation_started(self):
        self.disable_editing_notifier.notify_subscribers()
        self.view.enable_cancel()

    def handle_calculation_error(self, error):
        self.enable_editing_notifier.notify_subscribers()
        self.view.warning_popup(error)
        self.view.disable_cancel()
        self.current_alg = None

    def handle_calculation_success(self):
        self.phase_table_calculation_complete_notifier.notify_subscribers()
        self.enable_editing_notifier.notify_subscribers()
        self.update_current_phase_tables()
        self.view.enable_widget()
        self.view.disable_cancel()
        self.current_alg = None

    def calculate_phase_table(self):
        parameters = self.create_parameters_for_cal_muon_phase_algorithm()
        fitting_workspace_name = get_fitting_workspace_name(parameters['DetectorTable']) \
            if self.view.output_fit_information else '__NotUsed'

        self.current_alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        detector_table, fitting_information = run_CalMuonDetectorPhases(parameters, self.current_alg, fitting_workspace_name)
        self.current_alg = None

        self.add_phase_table_to_ADS(detector_table)
        self.add_fitting_info_to_ADS_if_required(parameters['DetectorTable'], fitting_information)

        return parameters['DetectorTable']

    def add_phase_table_to_ADS(self, base_name):
        run = get_run_numbers_as_string_from_workspace_name(base_name, self.context.data_context.instrument)

        directory = get_base_data_directory(self.context, run)
        muon_workspace_wrapper = MuonWorkspaceWrapper(directory + base_name)
        muon_workspace_wrapper.show()

        self.context.phase_context.add_phase_table(muon_workspace_wrapper)

    def add_fitting_info_to_ADS_if_required(self, base_name, fit_workspace_name):
        if not self.view.output_fit_information:
            return

        muon_workspace_wrapper = MuonWorkspaceWrapper(fit_workspace_name)
        muon_workspace_wrapper.show()

    def create_parameters_for_cal_muon_phase_algorithm(self):
        parameters = {}

        parameters['FirstGoodData'] = self.context.phase_context.options_dict['first_good_time']
        parameters['LastGoodData'] = self.context.phase_context.options_dict['last_good_time']

        parameters['InputWorkspace'] = self.context.phase_context.options_dict['input_workspace']

        forward_group = self.context.phase_context.options_dict['forward_group']
        parameters['ForwardSpectra'] = self.context.group_pair_context[forward_group].detectors

        backward_group = self.context.phase_context.options_dict['backward_group']
        parameters['BackwardSpectra'] = self.context.group_pair_context[backward_group].detectors

        parameters['DetectorTable'] = get_phase_table_workspace_name(parameters['InputWorkspace'], forward_group,
                                                                     backward_group)

        return parameters

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

        if self.view.input_workspace == "":
            self.view.disable_widget()
        else:
            self.view.setEnabled(True)

    def update_current_groups_list(self):
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

    def update_current_phase_tables(self):
        phase_table_list = self.context.phase_context.get_phase_table_list(self.context.data_context.instrument)
        self.view.set_phase_table_combo_box(phase_table_list)

    def handle_first_good_data_changed(self):
        self._validate_data_changed(self.view.first_good_time, "First Good Data")

    def handle_last_good_data_changed(self):
        self._validate_data_changed(self.view.last_good_time, "Last Good Data")

    def _validate_data_changed(self, data, string):
        run = float(get_run_number_from_workspace_name(self.view.input_workspace,
                                                       self.context.data_context.instrument))
        last_good_time = self.context.last_good_data([run])
        first_good_time = self.context.first_good_data([run])

        if self.view.first_good_time > self.view.last_good_time:
            self.view.first_good_time = first_good_time
            self.view.last_good_time = last_good_time
            self.view.warning_popup("First Good Data cannot be greater than Last Good Data")
        elif data < first_good_time:
            self.view.first_good_time = first_good_time
            self.view.warning_popup(f"{string} cannot be smaller than {first_good_time}")
        elif data > last_good_time:
            self.view.last_good_time = last_good_time
            self.view.warning_popup(f"{string} cannot be greater than {last_good_time}")
