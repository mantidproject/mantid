from __future__ import (absolute_import, division, print_function)

import time

from Muon.GUI.Common.threading_worker import Worker
from Muon.GUI.Common.threading_manager import WorkerManager
from Muon.GUI.Common.home_tab.home_tab_presenter import Observer

class GroupingTabPresenter(object):
    """

    The grouping tab presenter is responsible for synchronizing the group and pair tables.
    """

    def __init__(self, view, model,
                 grouping_table_widget=None,
                 pairing_table_widget=None):
        self._view = view
        self._model = model

        self.grouping_table_widget = grouping_table_widget
        self.pairing_table_widget = pairing_table_widget

        self._view.on_clear_grouping_button_clicked(self.on_clear_requested)

        # Synchronize the two tables
        self._view.on_grouping_table_changed(self.pairing_table_widget.update_view_from_model)
        self._view.on_pairing_table_changed(self.grouping_table_widget.update_view_from_model)

        self._view.on_add_pair_requested(self.add_pair_from_grouping_table)

        self._view.set_description_text(self.text_for_description())

        self._view.on_update_button_clicked(self.disable_editing)

        self.thread_manager = None

        # monitors for loaded data changing
        self.loadObserver = GroupingTabPresenter.LoadObserver(self)

    def add_pair_from_grouping_table(self, name1, name2):
        """If user requests to add a pair from the grouping table."""
        pair = self._model.construct_empty_pair_with_group_names(name1, name2)
        self._model.add_pair(pair)
        self.pairing_table_widget.update_view_from_model()

    def text_for_description(self):
        # TODO :  implement automatic update for decsription.
        text = "EMU longitudinal (?? detectors)"
        return text

    def show(self):
        self._view.show()

    def disable_editing(self):
        print("Disabling editing")
        self.grouping_table_widget.disable_editing()
        self.pairing_table_widget.disable_editing()

        # TODO : Update here using threading
        if self.thread_manager:
            self.thread_manager.clear()
            self.thread_manager.deleteLater()
            self.thread_manager = None
        self.thread_manager = WorkerManager(fn=self.timing, num_threads=1,
                                            callback_on_threads_complete=self.enable_editing, time_s=[2, 2, 2])
        self.thread_manager.start()
        #self.enable_editing()

    def timing(self, time_s):
        time.sleep(time_s)
        return

    def enable_editing(self):
        print("Enabling editing")
        self.grouping_table_widget.enable_editing()
        self.pairing_table_widget.enable_editing()

    def on_clear_requested(self):
        self._model.clear()
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()


    def handle_new_data_loaded(self):
        print("handle_new_data_loaded")
        print(self._model._data._groups.keys())
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()

    class LoadObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self, observable, arg):
            print("update called")
            self.outer.handle_new_data_loaded()