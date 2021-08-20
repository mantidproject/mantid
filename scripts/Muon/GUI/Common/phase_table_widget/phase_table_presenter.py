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
from Muon.GUI.Common.phase_table_widget.phase_table_view import REAL_PART, IMAGINARY_PART
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
        self.phase_table_observer = GenericObserver(self.update_current_phase_tables)
        self.phasequad_calculation_complete_notifier = Observable()
        self.enable_editing_notifier = Observable()
        self.disable_editing_notifier = Observable()

        self.disable_tab_observer = GenericObserver(self.view.disable_widget)
        self.enable_tab_observer = GenericObserver(self.view.enable_widget)
        self.selected_phasequad_changed_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)
        self.update_current_phase_tables()

        self.view.on_first_good_data_changed(self.handle_first_good_data_changed)
        self.view.on_last_good_data_changed(self.handle_last_good_data_changed)
        self.view.on_phasequad_table_data_changed(self.handle_phasequad_table_data_changed)

    def update_view_from_model(self):
        self.view.disable_updates()
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_current_phase_tables()
        for key, item in self.context.phase_context.options_dict.items():
            setattr(self.view, key, item)
        self.view.enable_updates()

    def update_model_from_view(self):
        for key in self.context.phase_context.options_dict:
            self.context.phase_context.options_dict[key] = getattr(self.view, key, None)

    def validate_phasequad_name(self, name):
        """Checks if name is in use by another pair of group, and that it is in a valid format"""
        if name in self.context.group_pair_context.pairs:
            self.view.warning_popup(
                "Groups and pairs (including phasequads) must have unique names")
            return False
        if not re.match(valid_name_regex, name):
            self.view.warning_popup(
                "Phasequad names should only contain digits, characters and _")
            return False
        return True

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.context.getGroupedWorkspaceNames())
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

        if self.view.input_workspace == "":
            self.view.disable_widget()
            self.view.clear_phase_tables()
            self.view.clear_phasequads()
        else:
            self.view.setEnabled(True)

    def update_current_groups_list(self):
        self.view.set_group_combo_boxes(self.context.group_pair_context.group_names)
        self.update_model_from_view()

    def cancel_current_alg(self):
        """Cancels the current algorithm if executing"""
        if self.current_alg is not None:
            self.current_alg.cancel()

    def handle_thread_calculation_started(self):
        """Generic handling of starting calculation threads"""
        self.disable_editing_notifier.notify_subscribers()

    def handle_thread_calculation_success(self):
        """Generic handling of success from calculation threads"""
        self.enable_editing_notifier.notify_subscribers()
        self.view.enable_widget()
        self.current_alg = None

    def handle_thread_calculation_error(self, error):
        """Generic handling of error from calculation threads"""
        self.enable_editing_notifier.notify_subscribers()
        self.view.warning_popup(error)
        self.current_alg = None

    """=============== Phase table methods ==============="""

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
        return parameters

    def update_current_phase_tables(self):
        """Retrieves up-to-date list of phase tables from context and adds to view"""
        phase_table_list = self.context.phase_context.get_phase_table_list(self.context.data_context.instrument)
        self.view.set_phase_table_combo_box(phase_table_list)

    def calculate_phase_table(self):
        """Runs the algorithm to create a phase table"""
        parameters = self.create_parameters_for_cal_muon_phase_algorithm()
        fitting_workspace_name = get_fitting_workspace_name(parameters['DetectorTable']) \
            if self.view.output_fit_information else '__NotUsed'

        self.current_alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        detector_table, fitting_information = run_CalMuonDetectorPhases(parameters, self.current_alg, fitting_workspace_name)
        self.current_alg = None

        self.add_phase_table_to_ADS(detector_table)
        self.add_fitting_info_to_ADS_if_required(parameters['DetectorTable'], fitting_information)

        return parameters['DetectorTable']

    def handle_phase_table_calculation_started(self):
        """Specific handling when calculating phase table is started"""
        self.view.enable_phase_table_cancel()
        self.handle_thread_calculation_started()

    def handle_phase_table_calculation_success(self):
        """Specific handling when a phase table calculation succeeds"""
        self.phase_table_calculation_complete_notifier.notify_subscribers()
        self.update_current_phase_tables()
        self.view.disable_phase_table_cancel()
        self._new_table_name = ""
        self.handle_thread_calculation_success()

    def handle_phase_table_calculation_error(self, error):
        """Specific handling when calculate phase table errors"""
        self.view.disable_phase_table_cancel()
        self.handle_thread_calculation_error(error)

    def create_phase_table_calculation_thread(self):
        self._calculation_model = ThreadModelWrapper(self.calculate_phase_table)
        return thread_model.ThreadModel(self._calculation_model)

    def handle_calculate_phase_table_clicked(self):
        self.update_model_from_view()
        self.disable_editing_notifier.notify_subscribers()

        # Get name of table, an empty string uses a default name so None is used to identify if cancel is pressed
        name = self.view.enter_phase_table_name()
        if name is None:
            self.enable_editing_notifier.notify_subscribers()
            return
        self._new_table_name = name

        # Calculate the new table in a separate thread
        self.calculation_thread = self.create_phase_table_calculation_thread()
        self.calculation_thread.threadWrapperSetUp(self.handle_phase_table_calculation_started,
                                                   self.handle_phase_table_calculation_success,
                                                   self.handle_phase_table_calculation_error)
        self.calculation_thread.start()

    def handle_phase_table_changed(self):
        """Handles when phase table is changed, recalculates any existing phasequads"""
        self.disable_editing_notifier.notify_subscribers()
        self.view.disable_widget()
        current_table = self.context.phase_context.options_dict['phase_table_for_phase_quad']
        new_table = self.view.get_phase_table()
        if new_table == current_table:
            return
        self.context.phase_context.options_dict['phase_table_for_phase_quad'] = new_table

        # Update the table stored in each phasequad
        self.context.group_pair_context.update_phase_tables(new_table)
        self.context.calculate_all_pairs()  # Updates phasequads
        self.calculation_finished_notifier.notify_subscribers()
        self.view.enable_widget()
        self.enable_editing_notifier.notify_subscribers()

    """=============== Phasequad methods ==============="""

    def calculate_phasequad(self):
        self.context.group_pair_context.add_phasequad(self._phasequad_obj)
        self.context.calculate_phasequads(
            self._phasequad_obj.name, self._phasequad_obj)

        self.phasequad_calculation_complete_notifier.notify_subscribers(
            self._phasequad_obj.Re.name)
        self.phasequad_calculation_complete_notifier.notify_subscribers(
            self._phasequad_obj.Im.name)

    def remove_last_row(self):
        if self.view.num_rows() > 0:
            name = self.view.get_table_contents()[-1]
            self.view.remove_last_row()
            for phasequad in self.context.group_pair_context.phasequads:
                if phasequad.name == name:
                    self.add_phasequad_to_analysis(False, False, phasequad)
                    self.context.group_pair_context.remove_phasequad(phasequad)
                    self.calculation_finished_notifier.notify_subscribers()

    def remove_selected_rows(self, phasequad_names):
        for name, index in reversed(phasequad_names):
            self.view.remove_phasequad_by_index(index)
            for phasequad in self.context.group_pair_context.phasequads:
                if phasequad.name == name:
                    self.add_phasequad_to_analysis(False, False, phasequad)
                    self.context.group_pair_context.remove_phasequad(phasequad)
                    self.calculation_finished_notifier.notify_subscribers()

    def handle_phasequad_calculation_started(self):
        """Specific handling when calculating phasequad is started"""
        self.handle_thread_calculation_started()

    def handle_phasequad_calculation_success(self):
        """Specific handling when a phasequad calculation succeeds"""
        self.add_phasequad_to_analysis(True, True, self._phasequad_obj)
        self.view.disable_updates()
        self.view.add_phasequad_to_table(self._phasequad_obj.name)
        self.view.enable_updates()
        self._phasequad_obj = None
        self.calculation_finished_notifier.notify_subscribers()
        self.handle_thread_calculation_success()

    def handle_phasequad_calculation_error(self, error):
        """Specific handling when calculate phase table errors"""
        self.handle_thread_calculation_error(error)

    def create_phasequad_calculation_thread(self):
        self._phasequad_calculation_model = ThreadModelWrapper(self.calculate_phasequad)
        return thread_model.ThreadModel(self._phasequad_calculation_model)

    def handle_add_phasequad_button_clicked(self):
        """When the + button is pressed, calculate a new phasequad from the currently selected table"""
        if self.view.number_of_phase_tables < 1:
            self.view.warning_popup("Please generate a phase table first.")
            return

        self.update_model_from_view()

        name = self.view.enter_phasequad_name()
        if name is None:
            return

        elif self.validate_phasequad_name(name):
            table = self.context.phase_context.options_dict['phase_table_for_phase_quad']
            self._phasequad_obj = MuonPhasequad(str(name), table)
            self.phasequad_calculation_thread = self.create_phasequad_calculation_thread()
            self.phasequad_calculation_thread.threadWrapperSetUp(self.handle_phasequad_calculation_started,
                                                                 self.handle_phasequad_calculation_success,
                                                                 self.handle_phasequad_calculation_error)
            self.phasequad_calculation_thread.start()

    def handle_remove_phasequad_button_clicked(self):
        phasequads = self.view.get_selected_phasequad_names_and_indexes()
        if not phasequads:
            self.remove_last_row()
        else:
            self.remove_selected_rows(phasequads)

    def handle_phasequad_table_data_changed(self, row, col):
        """Handles when either Analyse checkbox is changed"""
        item = self.view.get_table_item(row, col)
        name = self.view.get_table_item_text(row, 0)
        is_added = True if item.checkState() == 2 else False
        for phasequad in self.context.group_pair_context.phasequads:
            if phasequad.name == name:
                if col == REAL_PART:
                    self.add_part_to_analysis(is_added, phasequad.Re.name)
                elif col == IMAGINARY_PART:
                    self.add_part_to_analysis(is_added, phasequad.Im.name)

    def add_phasequad_to_analysis(self, re_is_added, im_is_added, phasequad):
        """Adds both Real and Imaginary parts to the analysis if their is_added is true, else removes them"""
        self.add_part_to_analysis(re_is_added, phasequad.Re.name, False)
        self.add_part_to_analysis(im_is_added, phasequad.Im.name, False)

    def add_part_to_analysis(self, is_added, name, notify=True):
        """Adds the Real (Re) or Imaginary (Im) part to the analysis if is_added is True, else removes it"""
        if is_added:
            self.context.group_pair_context.add_pair_to_selected_pairs(name)
        else:
            self.context.group_pair_context.remove_pair_from_selected_pairs(name)

        # Notify a new phasequad is selected to update the plot
        if notify:
            group_info = {'is_added': is_added, 'name': name}
            self.selected_phasequad_changed_notifier.notify_subscribers(group_info)
