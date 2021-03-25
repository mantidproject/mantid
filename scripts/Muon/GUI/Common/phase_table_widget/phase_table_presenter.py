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
        self._new_table_name = ""

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
        self.view.on_phase_quad_table_data_changed(self.handle_phasequad_table_data_changed)

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

    def handle_calculate_phase_table_clicked(self):
        self.update_model_from_view()
        self.disable_editing_notifier.notify_subscribers()

        # Get name if wanting to use name
        name = self.view.enter_phase_table_name()
        if name is None:  # Have to specify None as empty string is valid
            self.enable_editing_notifier.notify_subscribers()
            return
        self._new_table_name = name

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

    def create_phase_quads_calculation_thread(self, phasequads):
        self._phasequad_calculation_model = ThreadModelWrapper(self.calculate_phase_quads(phasequads))
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
        self.current_alg = None
        self.add_phase_quad_to_analysis(True, self._phasequad_obj)

        # Add to table
        self.view.add_phase_quad_to_table(self._phasequad_obj.name)
        self._phasequad_obj = None

    def handle_calculation_started(self):
        self.disable_editing_notifier.notify_subscribers()

    def handle_phase_table_calculation_started(self):
        self.disable_editing_notifier.notify_subscribers()
        self.view.enable_phase_table_cancel()

    def handle_calculation_error(self, error):
        self.enable_editing_notifier.notify_subscribers()
        self.view.warning_popup(error)
        self.view.disable_phase_table_cancel()
        self.current_alg = None

    def handle_calculation_success(self):
        self.phase_table_calculation_complete_notifier.notify_subscribers()
        self.enable_editing_notifier.notify_subscribers()
        self.update_current_phase_tables()
        self.view.enable_widget()
        self.view.disable_phase_table_cancel()
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
                                                                 backward_group, new_name=self._new_table_name)
        self._new_table_name = ""

        return parameters

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

        if self.view.input_workspace == "":
            self.view.disable_widget()
            # Clear phasequads
            _ = self.clear_phase_quads()
            self.view.clear_phase_tables()
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

    # Phasequad Table Functionality
    def to_analyse_data_checkbox_changed(self, state, row, phasequad_name):
        is_added = True if state == 2 else False
        for phasequad in self.context.group_pair_context.phasequads:
            if phasequad.name == phasequad_name:
                self.add_phase_quad_to_analysis(is_added, phasequad)

    def handle_phasequad_table_data_changed(self, row, col):
        item = self.view.get_table_item(row, col)
        name = self.view.get_table_item_text(row, 0)
        if col == 1:
            self.to_analyse_data_checkbox_changed(item.checkState(), row, name)

    def handle_remove_phasequad_button_clicked(self):
        self.remove_last_row()

    def remove_last_row(self):
        if self.view.num_rows() > 0:
            name = self.view.get_table_item_text(self.view.num_rows()-1, 0)
            self.view.remove_last_row()
            for phasequad in self.context.group_pair_context.phasequads:
                if phasequad.name == name:
                    self.add_phase_quad_to_analysis(False, phasequad)
                    self.context.group_pair_context.remove_phasequad(phasequad)

    def handle_phase_table_changed(self):
        current_table = self.context.phase_context.options_dict['phase_table_for_phase_quad']
        new_table = self.view.get_phase_table()
        if not new_table or not current_table or current_table == new_table:
            return  # No need to update
        self.context.phase_context.options_dict['phase_table_for_phase_quad'] = new_table
        # clear what is there currently and recalculate with new table
        if self.context.group_pair_context.phasequads:
            for i, phasequad in enumerate(self.context.group_pair_context.phasequads):
                self.add_phase_quad_to_analysis(False, phasequad)
                self.context.group_pair_context.phasequads[i].phase_table = new_table
                # Need to recalculate phasequads with new table
                self.context.calculate_phasequads(phasequad.name, phasequad)
                self.add_phase_quad_to_analysis(True, phasequad)
            self.view.set_phase_table(new_table)

    def clear_phase_quads(self):
        # Remove from view
        old_names = self.view.clear_phase_quads()
        # Remove from analysis and context
        for phaseqaud in self.context.group_pair_context.phasequads:
            self.add_phase_quad_to_analysis(False, phaseqaud)
            self.context.group_pair_context.remove_phasequad(phaseqaud)
        return old_names

    def add_phase_quad_to_analysis(self, is_added, phasequad):
        names = [phasequad.Re.name, phasequad.Im.name]
        for name in names:
            if is_added:
                self.context.group_pair_context.add_pair_to_selected_pairs(
                    name)
            else:
                self.context.group_pair_context.remove_pair_from_selected_pairs(
                    name)
            group_info = {'is_added': is_added, 'name': name}
            self.selected_phasequad_changed_notifier.notify_subscribers(
                group_info)

    def calculate_phase_quads(self, phasequads):
        table = self.context.phase_context.options_dict['phase_table_for_phase_quad']
        for name in phasequads:
            phasequad = MuonPhasequad(name, table)
            self.context.group_pair_context.add_phasequad(phasequad)

            self.context.calculate_phasequads(
                phasequad.name, phasequad)
            self.phase_quad_calculation_complete_notifier.notify_subscribers(
                phasequad.Re.name)
            self.phase_quad_calculation_complete_notifier.notify_subscribers(
                phasequad.Im.name)

    def handle_success(self):
        self.enable_editing_notifier.notify_subscribers()
        self.view.enable_widget()
        self.current_alg = None
        for phasequad in self.context.group_pair_context.phasequads:
            self.add_phase_quad_to_analysis(True, phasequad)

            # Add to table
            self.view.add_phase_quad_to_table(phasequad.name)
