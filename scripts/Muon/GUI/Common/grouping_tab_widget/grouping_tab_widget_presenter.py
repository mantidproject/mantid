from __future__ import (absolute_import, division, print_function)

import time

from Muon.GUI.Common.threading_worker import Worker
from Muon.GUI.Common.threading_manager import WorkerManager
from Muon.GUI.Common.home_tab.home_tab_presenter import Observer
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import Observable
from Muon.GUI.Common.home_tab.home_tab_presenter import Observer

import Muon.GUI.Common.muon_file_utils as file_utils
import Muon.GUI.Common.load_utils as load_utils


class GroupingTabPresenter(object):
    """
    The grouping tab presenter is responsible for synchronizing the group and pair tables. It also maintains
    functionality which covers both groups/pairs ; e.g. loading/saving/updating data.
    """

    @staticmethod
    def text_for_description():
        # TODO :  implement automatic update for decsription.
        text = "EMU longitudinal (?? detectors)"
        return text

    @staticmethod
    def timing(time_s):
        time.sleep(time_s)
        return

    def __init__(self, view, model,
                 grouping_table_widget=None,
                 pairing_table_widget=None):
        self._view = view
        self._model = model

        self.grouping_table_widget = grouping_table_widget
        self.pairing_table_widget = pairing_table_widget

        # Synchronize the two tables
        self._view.on_grouping_table_changed(self.pairing_table_widget.update_view_from_model)
        self._view.on_pairing_table_changed(self.grouping_table_widget.update_view_from_model)

        self._view.set_description_text(self.text_for_description())
        self._view.on_add_pair_requested(self.add_pair_from_grouping_table)
        self._view.on_clear_grouping_button_clicked(self.on_clear_requested)
        self._view.on_update_button_clicked(self.handle_update_all_clicked)
        self._view.on_load_grouping_button_clicked(self.handle_load_grouping_from_file)
        self._view.on_save_grouping_button_clicked(self.handle_save_grouping_file)
        self._view.on_default_grouping_button_clicked(self.handle_default_grouping_button_clicked)

        # multi-threading
        self.thread_manager = None

        # monitors for loaded data changing
        self.loadObserver = GroupingTabPresenter.LoadObserver(self)
        self.instrumentObserver = GroupingTabPresenter.InstrumentObserver(self)

        # notifiers
        self.groupingNotifier = GroupingTabPresenter.GroupingNotifier(self)
        self.grouping_table_widget.on_data_changed(self.group_table_changed)
        self.pairing_table_widget.on_data_changed(self.pair_table_changed)

        self.guessAlphaObserver = GroupingTabPresenter.GuessAlphaObserver(self)
        self.pairing_table_widget.guessAlphaNotifier.add_subscriber(self.guessAlphaObserver)

    def show(self):
        self._view.show()

    def group_table_changed(self):
        self.groupingNotifier.notify_subscribers()

    def pair_table_changed(self):
        self.groupingNotifier.notify_subscribers()

    def add_pair_from_grouping_table(self, name1, name2):
        """If user requests to add a pair from the grouping table."""
        pair = self._model.construct_empty_pair_with_group_names(name1, name2)
        self._model.add_pair(pair)
        self.pairing_table_widget.update_view_from_model()

    def handle_guess_alpha(self, pair_name, group1_name, group2_name):
        ws1 = self._model.get_group_workspace(group1_name)
        ws2 = self._model.get_group_workspace(group2_name)

        ws = load_utils.run_AppendSpectra(ws1, ws2)

        new_alpha = load_utils.run_AlphaCalc({"InputWorkspace": ws,
                                              "ForwardSpectra": [0],
                                              "BackwardSpectra": [1]})
        self._model.update_pair_alpha(pair_name, new_alpha)
        self.pairing_table_widget.update_view_from_model()

    def handle_load_grouping_from_file(self):
        file_filter = file_utils.filter_for_extensions(["xml"])
        directory = ""
        filename = self._view.show_file_browser_and_return_selection(file_filter, [directory])

        groups, pairs = load_utils.load_grouping_from_XML(filename)

        self._model.clear()

        for group in groups:
            self._model.add_group(group)

        for pair in pairs:
            self._model.add_pair(pair)

        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()

    def disable_editing(self):
        self._view.set_buttons_enabled(False)
        self.grouping_table_widget.disable_editing()
        self.pairing_table_widget.disable_editing()

    def calculate_all_data(self, arg):
        self._model._data.show_all_groups()
        self._model._data.show_all_pairs()

    def handle_update_all_clicked(self):
        self.disable_editing()
        # TODO : Update here using threading
        if self.thread_manager:
            self.thread_manager.clear()
            self.thread_manager.deleteLater()
            self.thread_manager = None
        self.thread_manager = WorkerManager(fn=self.calculate_all_data, num_threads=1,
                                            callback_on_threads_complete=self.enable_editing, arg=[1])
        self.thread_manager.start()

    def handle_default_grouping_button_clicked(self):
        #self._model.clear()
        self._model._data.set_groups_and_pairs_to_default()
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()

    def enable_editing(self):
        self._view.set_buttons_enabled(True)
        self.grouping_table_widget.enable_editing()
        self.pairing_table_widget.enable_editing()

    def on_clear_requested(self):
        self._model.clear()
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()

    def handle_new_data_loaded(self):
        if self._model._data.is_data_loaded():
            self.grouping_table_widget.update_view_from_model()
            self.pairing_table_widget.update_view_from_model()
        else:
            self.on_clear_requested()

    def handle_save_grouping_file(self):
        filename = self._view.show_file_save_browser_and_return_selection()
        load_utils.save_grouping_to_XML(self._model.groups, self._model.pairs, filename)

    class LoadObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_new_data_loaded()

    class InstrumentObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self, observable, arg):
            self.outer.on_clear_requested()

    class GroupingNotifier(Observable):

        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, arg=None):
            Observable.notify_subscribers(self, arg)

    class GuessAlphaObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self, observable, arg=["", "", ""]):
            self.outer.handle_guess_alpha(arg[0], arg[1], arg[2])
