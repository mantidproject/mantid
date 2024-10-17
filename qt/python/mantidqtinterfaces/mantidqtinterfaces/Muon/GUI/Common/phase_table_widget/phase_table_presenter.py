# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from mantidqtinterfaces.Muon.GUI.Common import thread_model
from mantidqtinterfaces.Muon.GUI.Common.muon_phasequad import MuonPhasequad
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_view import REAL_PART, IMAGINARY_PART
from mantidqt.utils.observer_pattern import Observable, GenericObserver, GenericObservable
import re
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import get_fitting_workspace_name, get_run_number_from_workspace_name
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import valid_name_regex


class PhaseTablePresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model

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
        self.view.set_input_combo_box(self.model.get_grouped_workspace_names())
        self.view.set_group_combo_boxes(self.model.group_pair_names)
        self.update_current_phase_tables()
        for key, item in self.model.phase_context.options_dict.items():
            setattr(self.view, key, item)
        self.view.enable_updates()

    def update_model_from_view(self):
        for key in self.model.phase_context.options_dict:
            self.model.phase_context.options_dict[key] = getattr(self.view, key, None)

    def validate_phasequad_name(self, name):
        """Checks if name is in use by another pair of group, and that it is in a valid format"""
        if name in self.model.group_pairs:
            self.view.warning_popup("Groups and pairs (including phasequads) must have unique names")
            return False
        if not re.match(valid_name_regex, name):
            self.view.warning_popup("Phasequad names should only contain digits, characters and _")
            return False
        return True

    def update_current_run_list(self):
        self.view.set_input_combo_box(self.model.get_grouped_workspace_names())
        self.view.set_group_combo_boxes(self.model.group_pair_names)
        self.update_model_from_view()

        if self.view.input_workspace == "":
            self.view.disable_widget()
            self.view.clear_phase_tables()
            self.view.clear_phasequads()
        else:
            self.view.setEnabled(True)

    def update_current_groups_list(self):
        self.view.set_group_combo_boxes(self.model.group_pair_names)
        self.update_model_from_view()

    def cancel_current_alg(self):
        """Cancels the current algorithm if executing"""
        self.model.cancel_current_alg()

    def handle_thread_calculation_started(self):
        """Generic handling of starting calculation threads"""
        self.disable_editing_notifier.notify_subscribers()

    def handle_thread_calculation_success(self):
        """Generic handling of success from calculation threads"""
        self.enable_editing_notifier.notify_subscribers()
        self.view.enable_widget()
        self.model.clear_current_alg()

    def handle_thread_calculation_error(self, error):
        """Generic handling of error from calculation threads"""
        self.enable_editing_notifier.notify_subscribers()
        self.view.warning_popup(error.exc_value)
        self.model.clear_current_alg()

    """=============== Phase table methods ==============="""

    def handle_first_good_data_changed(self):
        self._validate_data_changed(self.view.first_good_time, "First Good Data")

    def handle_last_good_data_changed(self):
        self._validate_data_changed(self.view.last_good_time, "Last Good Data")

    def _validate_data_changed(self, data, string):
        run = float(get_run_number_from_workspace_name(self.view.input_workspace, self.model.instrument))
        last_good_time = self.model.context.last_good_data([run])
        first_good_time = self.model.context.first_good_data([run])

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

    def add_fitting_info_to_ADS_if_required(self, fit_workspace_name):
        if not self.view.output_fit_information:
            return

        self.model.add_fitting_info_to_ads(fit_workspace_name)

    def update_current_phase_tables(self):
        """Retrieves up-to-date list of phase tables from context and adds to view"""
        phase_table_list = self.model.phase_context.get_phase_table_list(self.model.instrument)
        self.view.set_phase_table_combo_box(phase_table_list)

    def calculate_phase_table(self):
        """Runs the algorithm to create a phase table"""
        parameters = self.model.create_parameters_for_cal_muon_phase_algorithm()
        fitting_workspace_name = (
            get_fitting_workspace_name(parameters["DetectorTable"]) if self.view.output_fit_information else "__NotUsed"
        )

        detector_table, fitting_information = self.model.CalMuonDetectorPhases_wrapper(parameters, fitting_workspace_name)

        self.model.add_phase_table_to_ads(detector_table)
        self.add_fitting_info_to_ADS_if_required(fitting_information)

        return parameters["DetectorTable"]

    def handle_phase_table_calculation_started(self):
        """Specific handling when calculating phase table is started"""
        self.view.enable_phase_table_cancel()
        self.handle_thread_calculation_started()

    def handle_phase_table_calculation_success(self):
        """Specific handling when a phase table calculation succeeds"""
        self.phase_table_calculation_complete_notifier.notify_subscribers()
        self.update_current_phase_tables()
        self.view.disable_phase_table_cancel()
        self.model.new_table_name = ""
        self.handle_thread_calculation_success()

    def handle_phase_table_calculation_error(self, error):
        """Specific handling when calculate phase table errors"""
        self.view.disable_phase_table_cancel()
        self.handle_thread_calculation_error(error)

    def create_phase_table_calculation_thread(self):
        calculation_model = ThreadModelWrapper(self.calculate_phase_table)
        return thread_model.ThreadModel(calculation_model)

    def handle_calculate_phase_table_clicked(self):
        self.update_model_from_view()
        self.disable_editing_notifier.notify_subscribers()

        # Get name of table, an empty string uses a default name so None is used to identify if cancel is pressed
        name = self.view.enter_phase_table_name()
        if name is None:
            self.enable_editing_notifier.notify_subscribers()
            return
        self.model.new_table_name = name

        # Calculate the new table in a separate thread
        self.model.calculation_thread = self.create_phase_table_calculation_thread()
        self.model.calculation_thread.threadWrapperSetUp(
            self.handle_phase_table_calculation_started,
            self.handle_phase_table_calculation_success,
            self.handle_phase_table_calculation_error,
        )
        self.model.calculation_thread.start()

    def handle_phase_table_changed(self):
        """Handles when phase table is changed, recalculates any existing phasequads"""
        self.disable_editing_notifier.notify_subscribers()
        self.view.disable_widget()
        current_table = self.model.phase_context.options_dict["phase_table_for_phase_quad"]
        new_table = self.view.get_phase_table()
        if new_table == current_table:
            return
        self.model.phase_context.options_dict["phase_table_for_phase_quad"] = new_table

        # Update the table stored in each phasequad
        self.model.group_pair_context.update_phase_tables(new_table)
        self.model.context.update_phasequads()  # Updates phasequads
        self.calculation_finished_notifier.notify_subscribers()
        self.view.enable_widget()
        self.enable_editing_notifier.notify_subscribers()

    """=============== Phasequad methods ==============="""

    def calculate_phasequad(self):
        self.model.group_pair_context.add_phasequad(self.model.phasequad)
        self.model.context.calculate_phasequads(self.model.phasequad)

        self.phasequad_calculation_complete_notifier.notify_subscribers(self.model.phasequad.Re.name)
        self.phasequad_calculation_complete_notifier.notify_subscribers(self.model.phasequad.Im.name)

    def remove_last_row(self):
        if self.view.num_rows() > 0:
            name = self.view.get_table_contents()[-1]
            self.view.remove_last_row()
            for phasequad in self.model.group_phasequads:
                if phasequad.name == name:
                    self.add_phasequad_to_analysis(False, False, phasequad)
                    self.model.remove_phasequad(phasequad)
                    self.calculation_finished_notifier.notify_subscribers()

    def remove_selected_rows(self, phasequad_names):
        for name, index in reversed(phasequad_names):
            self.view.remove_phasequad_by_index(index)
            for phasequad in self.model.group_phasequads:
                if phasequad.name == name:
                    self.add_phasequad_to_analysis(False, False, phasequad)
                    self.model.remove_phasequad(phasequad)
                    self.calculation_finished_notifier.notify_subscribers()

    def handle_phasequad_calculation_started(self):
        """Specific handling when calculating phasequad is started"""
        self.handle_thread_calculation_started()

    def handle_phasequad_calculation_success(self):
        """Specific handling when a phasequad calculation succeeds"""
        self.add_phasequad_to_analysis(True, True, self.model.phasequad)
        self.view.disable_updates()
        self.view.add_phasequad_to_table(self.model.phasequad.name)
        self.view.enable_updates()
        self.model.phasequad = None
        self.calculation_finished_notifier.notify_subscribers()
        self.handle_thread_calculation_success()

    def handle_phasequad_calculation_error(self, error):
        """Specific handling when calculate phase table errors"""
        self.handle_thread_calculation_error(error)

    def create_phasequad_calculation_thread(self):
        phasequad_calculation_model = ThreadModelWrapper(self.calculate_phasequad)
        return thread_model.ThreadModel(phasequad_calculation_model)

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
            table = self.model.phase_context.options_dict["phase_table_for_phase_quad"]
            self.model.phasequad = MuonPhasequad(str(name), table)
            self.model.phasequad_calculation_thread = self.create_phasequad_calculation_thread()
            self.model.phasequad_calculation_thread.threadWrapperSetUp(
                self.handle_phasequad_calculation_started,
                self.handle_phasequad_calculation_success,
                self.handle_phasequad_calculation_error,
            )
            self.model.phasequad_calculation_thread.start()

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
        for phasequad in self.model.group_phasequads:
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
            self.model.group_pair_context.add_pair_to_selected_pairs(name)
        else:
            self.model.group_pair_context.remove_pair_from_selected_pairs(name)

        # Notify a new phasequad is selected to update the plot
        if notify:
            group_info = {"is_added": is_added, "name": name}
            self.selected_phasequad_changed_notifier.notify_subscribers(group_info)
