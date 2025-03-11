# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import (
    Observer,
    Observable,
    GenericObservable,
    GenericObserver,
)
from mantidqt.widgets.muonperiodinfo import MuonPeriodInfo
import mantidqtinterfaces.Muon.GUI.Common.utilities.muon_file_utils as file_utils
from mantidqtinterfaces.Muon.GUI.Common.utilities.general_utils import round_value
import mantidqtinterfaces.Muon.GUI.Common.utilities.xml_utils as xml_utils
import mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils as algorithm_utils
from mantidqtinterfaces.Muon.GUI.Common import thread_model
from mantidqtinterfaces.Muon.GUI.Common.run_selection_dialog import RunSelectionDialog
from mantidqtinterfaces.Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import (
    run_string_to_list,
)

import os


class GroupingTabPresenter(object):
    """
    The grouping tab presenter is responsible for synchronizing the group and pair tables. It also maintains
    functionality which covers both groups/pairs ; e.g. loading/saving/updating data.
    """

    @staticmethod
    def string_to_list(text):
        return run_string_to_list(text)

    def __init__(
        self,
        view,
        model,
        grouping_table_widget=None,
        pairing_table_widget=None,
        diff_table=None,
        parent=None,
    ):
        self._view = view
        self._model = model

        self.grouping_table_widget = grouping_table_widget
        self.pairing_table_widget = pairing_table_widget
        self.diff_table = diff_table
        self.period_info_widget = MuonPeriodInfo()

        self._view.set_description_text("")
        self._view.on_add_pair_requested(self.add_pair_from_grouping_table)
        self._view.on_clear_grouping_button_clicked(self.on_clear_requested)
        self._view.on_load_grouping_button_clicked(self.handle_load_grouping_from_file)
        self._view.on_save_grouping_button_clicked(self.handle_save_grouping_file)
        self._view.on_default_grouping_button_clicked(self.handle_default_grouping_button_clicked)
        self._view.on_period_information_button_clicked(self.handle_period_information_button_clicked)

        # monitors for loaded data changing
        self.loadObserver = GroupingTabPresenter.LoadObserver(self)
        self.instrumentObserver = GroupingTabPresenter.InstrumentObserver(self)

        # notifiers
        self.groupingNotifier = GroupingTabPresenter.GroupingNotifier(self)
        self.grouping_table_widget.on_data_changed(self.group_table_changed)
        self.diff_table.on_data_changed(self.diff_table_changed)
        self.pairing_table_widget.on_data_changed(self.pair_table_changed)
        self.enable_editing_notifier = GroupingTabPresenter.EnableEditingNotifier(self)
        self.disable_editing_notifier = GroupingTabPresenter.DisableEditingNotifier(self)
        self.counts_calculation_finished_notifier = GenericObservable()

        self.guessAlphaObserver = GroupingTabPresenter.GuessAlphaObserver(self)
        self.pairing_table_widget.guessAlphaNotifier.add_subscriber(self.guessAlphaObserver)
        self.message_observer = GroupingTabPresenter.MessageObserver(self)
        self.gui_variables_observer = GroupingTabPresenter.GuiVariablesChangedObserver(self)
        self.enable_observer = GroupingTabPresenter.EnableObserver(self)
        self.disable_observer = GroupingTabPresenter.DisableObserver(self)

        self.disable_tab_observer = GenericObserver(self.disable_editing_without_notifying_subscribers)
        self.enable_tab_observer = GenericObserver(self.enable_editing_without_notifying_subscribers)

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)

    def update_view_from_model(self):
        self.grouping_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()

    def show(self):
        self._view.show()

    def text_for_description(self):
        """
        Generate the text for the description edit at the top of the widget.
        """
        instrument = self._model.instrument
        n_detectors = self._model.num_detectors
        main_field = self._model.main_field_direction
        text = "{}, {} detectors".format(instrument, n_detectors)
        if main_field:
            text += ", main field : {} to muon polarization".format(main_field)
        return text

    def update_description_text(self, description_text=""):
        if not description_text:
            description_text = self.text_for_description()
        self._view.set_description_text(description_text)

    def update_description_text_to_empty(self):
        self._view.set_description_text("")

    def add_pair_from_grouping_table(self, group_name1="", group_name2=""):
        """
        If user requests to add a pair from the grouping table.
        """
        self.pairing_table_widget.handle_add_pair_button_clicked(group_name1, group_name2)

    def handle_guess_alpha(self, pair_name, group1_name, group2_name):
        """
        Calculate alpha for the pair for which "Guess Alpha" button was clicked.
        """
        if len(self._model._data.current_runs) > 1:
            run, index, ok_clicked = RunSelectionDialog.get_run(self._model._data.current_runs, self._model._data.instrument, self._view)
            if not ok_clicked:
                return
            run_to_use = self._model._data.current_runs[index]
        else:
            run_to_use = self._model._data.current_runs[0]

        try:
            ws1 = self._model.get_group_workspace(group1_name, run_to_use)
            ws2 = self._model.get_group_workspace(group2_name, run_to_use)
        except KeyError:
            self._view.display_warning_box("Group workspace not found, try updating all and then recalculating.")
            return

        ws = algorithm_utils.run_AppendSpectra(ws1, ws2)

        new_alpha = algorithm_utils.run_AlphaCalc({"InputWorkspace": ws, "ForwardSpectra": [0], "BackwardSpectra": [1]})

        self._model.update_pair_alpha(
            pair_name,
            round_value(new_alpha, self._model._context.group_pair_context.alpha_precision),
        )
        self.pairing_table_widget.update_view_from_model()

        self.handle_update_all_clicked()

    def handle_load_grouping_from_file(self):
        # Only XML format
        file_filter = file_utils.filter_for_extensions(["xml"])
        filename = self._view.show_file_browser_and_return_selection(file_filter, [""])

        if filename == "":
            return

        groups, pairs, diffs, description, default = xml_utils.load_grouping_from_XML(filename)

        self._model.clear()
        for group in groups:
            try:
                self._model.add_group(group)
            except ValueError as error:
                self._view.display_warning_box(str(error))

        for pair in pairs:
            try:
                if pair.forward_group in self._model.group_names and pair.backward_group in self._model.group_names:
                    self._model.add_pair(pair)
            except ValueError as error:
                self._view.display_warning_box(str(error))
        for diff in diffs:
            try:
                if diff.positive in self._model.group_names and diff.negative in self._model.group_names:
                    self._model.add_diff(diff)
                elif diff.positive in self._model.pair_names and diff.negative in self._model.pair_names:
                    self._model.add_diff(diff)
            except ValueError as error:
                self._view.display_warning_box(str(error))
        # Sets the default from file if it exists, if not selected groups/pairs are set on the logic
        # Select all pairs if there are any pairs otherwise select all groups.
        if default:
            if default in self._model.group_names:
                self._model.add_group_to_analysis(default)
            elif default in self._model.pair_names:
                self._model.add_pair_to_analysis(default)

        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.update_description_text(description)
        self._model._context.group_pair_context.selected = default
        self.plot_default_groups_or_pairs()
        self.groupingNotifier.notify_subscribers()

        self.handle_update_all_clicked()

    def disable_editing(self):
        self._view.set_buttons_enabled(False)
        self.grouping_table_widget.disable_editing()
        self.diff_table.disable_editing()
        self.pairing_table_widget.disable_editing()
        self.disable_editing_notifier.notify_subscribers()

    def enable_editing(self, result=None):
        self._view.set_buttons_enabled(True)
        self.grouping_table_widget.enable_editing()
        self.diff_table.enable_editing()
        self.pairing_table_widget.enable_editing()
        self.enable_editing_notifier.notify_subscribers()

    def disable_editing_without_notifying_subscribers(self):
        self._view.set_buttons_enabled(False)
        self.grouping_table_widget.disable_editing()
        self.diff_table.disable_editing()
        self.pairing_table_widget.disable_editing()

    def enable_editing_without_notifying_subscribers(self):
        self._view.set_buttons_enabled(True)
        self.grouping_table_widget.enable_editing()
        self.diff_table.enable_editing()
        self.pairing_table_widget.enable_editing()

    def calculate_all_data(self):
        self._model.calculate_all_data()

    def handle_update_all_clicked(self):
        self.update_thread = self.create_update_thread()
        self.update_thread.threadWrapperSetUp(self.disable_editing, self.handle_update_finished, self.error_callback)
        self.update_thread.start()

    def error_callback(self, error_message):
        self.enable_editing()
        self._view.display_warning_box(error_message)

    def handle_update_finished(self):
        self.enable_editing()
        self.groupingNotifier.notify_subscribers()
        self.counts_calculation_finished_notifier.notify_subscribers()

    def handle_default_grouping_button_clicked(self):
        status = self._model.reset_groups_and_pairs_to_default()
        if status == "failed":
            self._view.display_warning_box("The default may depend on the instrument. Please load a run.")
            return
        self._model.reset_selected_groups_and_pairs()
        self.grouping_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.update_description_text()
        self.groupingNotifier.notify_subscribers()
        self.handle_update_all_clicked()
        self.plot_default_groups_or_pairs()

    def on_clear_requested(self):
        self._model.clear()
        self.grouping_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.update_description_text_to_empty()
        self.groupingNotifier.notify_subscribers()

    def handle_new_data_loaded(self):
        self.period_info_widget.clear()
        if self._model.is_data_loaded():
            self._model._context.show_raw_data()
            self.update_view_from_model()
            self.update_description_text()
            self.handle_update_all_clicked()
            self.plot_default_groups_or_pairs()
            if self.period_info_widget.isVisible():
                self._add_period_info_to_widget()
        else:
            self.on_clear_requested()

    def _check_and_get_filename(self, chosen_file):
        if chosen_file == "":
            return chosen_file

        path_extension = os.path.splitext(chosen_file)

        if path_extension[1] == ".xml":
            return chosen_file
        else:
            updated_file = path_extension[0] + ".xml"
            if os.path.isfile(updated_file):
                if self._view.show_question_dialog(updated_file):
                    return path_extension[0] + ".xml"
                else:
                    return ""
            return path_extension[0] + ".xml"

    def handle_save_grouping_file(self):
        chosen_file = self._view.get_save_filename()
        filename = self._check_and_get_filename(chosen_file)

        if filename != "":
            xml_utils.save_grouping_to_XML(
                self._model.groups,
                self._model.pairs,
                self._model.diffs,
                filename,
                description=self._view.get_description_text(),
            )

    def create_update_thread(self):
        self._update_model = ThreadModelWrapper(self.calculate_all_data)
        return thread_model.ThreadModel(self._update_model)

    def plot_default_groups_or_pairs(self):
        # if we have no pairs or groups selected, generate a default plot
        if len(self._model.selected_groups_and_pairs) == 0:
            if len(self._model.pairs) > 0:  # if we have pairs - then plot all pairs
                self.pairing_table_widget.plot_default_case()
            else:  # else plot groups
                self.grouping_table_widget.plot_default_case()

    def handle_period_information_button_clicked(self):
        if self._model.is_data_loaded() and self.period_info_widget.isEmpty():
            self._add_period_info_to_widget()
        self.period_info_widget.show()
        self.period_info_widget.raise_()

    def closePeriodInfoWidget(self):
        self.period_info_widget.close()

    def _add_period_info_to_widget(self):
        try:
            self.period_info_widget.addInfo(self._model._data.current_workspace)
            runs = self._model._data.current_runs
            runs_string = ""
            for run_list in runs:
                for run in run_list:
                    if runs_string:
                        runs_string += ", "
                    runs_string += str(run)
            self.period_info_widget.setWidgetTitleRuns(self._model.instrument + runs_string)
        except RuntimeError:
            self._view.display_warning_box("Could not read period info from the current workspace")

    # ------------------------------------------------------------------------------------------------------------------
    # Observer / Observable
    # ------------------------------------------------------------------------------------------------------------------

    def group_table_changed(self):
        self.pairing_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.handle_update_all_clicked()

    def pair_table_changed(self):
        self.grouping_table_widget.update_view_from_model()
        self.diff_table.update_view_from_model()
        self.handle_update_all_clicked()

    def diff_table_changed(self):
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.handle_update_all_clicked()

    class LoadObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_new_data_loaded()

    class InstrumentObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.on_clear_requested()

    class GuessAlphaObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_guess_alpha(arg[0], arg[1], arg[2])

    class GuiVariablesChangedObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_update_all_clicked()

    class GroupingNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args, **kwargs)

    class MessageObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer._view.display_warning_box(arg)

    class EnableObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.enable_editing()

    class DisableObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.disable_editing()

    class DisableEditingNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args, **kwargs)

    class EnableEditingNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args, **kwargs)
