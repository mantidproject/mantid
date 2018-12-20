# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import copy

from Muon.GUI.Common import thread_model
import Muon.GUI.Common.utilities.run_string_utils as run_utils
import Muon.GUI.Common.utilities.muon_file_utils as file_utils
import Muon.GUI.Common.utilities.load_utils as load_utils


class LoadRunWidgetPresenter(object):
    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._load_thread = None

        self._load_multiple_runs = False
        self._use_threading = True
        self._multiple_file_mode = "Co-add"

        self._instrument = ""
        self._view.set_current_instrument(self._instrument)

        self._set_connections()

    def _set_connections(self):
        self._view.on_load_current_run_clicked(self.handle_load_current_run)
        self._view.on_increment_run_clicked(self.handle_increment_run)
        self._view.on_decrement_run_clicked(self.handle_decrement_run)
        self._view.on_run_edit_changed(self.handle_run_changed_by_user)

    def show(self):
        self._view.show()

    # used with threading
    def create_load_thread(self):
        return thread_model.ThreadModel(self._model)

    # used with threading
    def cancel(self):
        if self._load_thread is not None:
            self._load_thread.cancel()

    def get_current_instrument(self):
        return str(self._instrument)

    def set_current_instrument(self, instrument):
        self._instrument = instrument
        self._view.set_current_instrument(instrument)

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear_loaded_data()

    def enable_multiple_files(self, enabled):
        self._load_multiple_runs = enabled

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def filenames(self):
        return self._model.loaded_filenames

    @property
    def runs(self):
        return self._model.loaded_runs

    # used by parent widget
    def update_view_from_model(self, run_list):
        self.set_run_edit_from_list(run_list)

    def update_multiple_loading_behaviour(self, text):
        self._multiple_file_mode = text

    def set_run_edit_from_list(self, run_list):
        new_list = []
        for run_item in run_list:
            if isinstance(run_item, int):
                new_list += [run_item]
            elif isinstance(run_item, list):
                for run in run_item:
                    new_list += [run]
        run_string = run_utils.run_list_to_string(new_list)
        self._view.set_run_edit_text(run_string)

    # ------------------------------------------------------------------------------------------------------------------
    # Loading from user input
    # ------------------------------------------------------------------------------------------------------------------

    def handle_run_changed_by_user(self):
        run_string = self._view.get_run_edit_text()
        run_list = run_utils.run_string_to_list(run_string)
        file_names = [file_utils.file_path_for_instrument_and_run(self.get_current_instrument(), new_run)
                      for new_run in run_list]

        if len(file_names) > 1 and not self._load_multiple_runs:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_run_edit_from_cache()
            return

        self.handle_loading(file_names, self._use_threading)

    def handle_loading(self, filenames, threaded=True):
        if threaded:
            self.handle_load_thread_start(filenames, self.handle_load_thread_finished)
        else:
            self.handle_load_no_threading(filenames, self.on_loading_finished)

    def handle_load_no_threading(self, filenames, finished_callback):
        self.on_loading_start()
        self._model.loadData(filenames)
        try:
            self._model.execute()
        except ValueError as error:
            self._view.warning_popup(error.args[0])
        finished_callback()

    def on_loading_start(self):
        self._view.notify_loading_started()
        self.disable_loading()

    def handle_load_thread_start(self, filenames, finished_callback):
        self.on_loading_start()

        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading,
                                             finished_callback,
                                             self._view.warning_popup)
        self._load_thread.loadData(filenames)
        self._load_thread.start()

    def handle_load_thread_finished(self):

        self._load_thread.deleteLater()
        self._load_thread = None

        self.on_loading_finished()

    def on_loading_finished(self):
        # If in single file mode, remove the previous run
        if not self._load_multiple_runs and len(self._model.loaded_runs) > 1:
            self._model.remove_previous_data()

        run_list = self._model.loaded_runs
        self.set_run_edit_from_list(run_list)

        if self._load_multiple_runs and self._multiple_file_mode == "Co-Add":
            load_utils.combine_loaded_runs(self._model, run_list)

        self._view.notify_loading_finished()
        self.enable_loading()

    # ------------------------------------------------------------------------------------------------------------------
    # Loading from current run button
    # ------------------------------------------------------------------------------------------------------------------

    def handle_load_current_run(self):

        try:
            current_run_file = file_utils.get_current_run_filename(self.get_current_instrument())
        except ValueError as error:
            self._view.warning_popup(error.args[0])
            return

        if current_run_file == "":
            self._view.warning_popup("Cannot find directory for current instrument : " + self._instrument)
            return

        self.handle_loading_current_run([current_run_file], self._use_threading)

    def handle_loading_current_run(self, filenames, threaded=True):
        if threaded:
            self.handle_load_thread_start(filenames, self.handle_load_thread_finished_current_run)
        else:
            self.handle_load_no_threading(filenames, self.on_loading_current_run_finished)

    def handle_load_thread_finished_current_run(self):

        self._load_thread.deleteLater()
        self._load_thread = None

        self.on_loading_current_run_finished()

    def on_loading_current_run_finished(self):
        # If in single file mode, remove the previous run
        if not self._load_multiple_runs and len(self._model.loaded_runs) > 1:
            self._model.remove_previous_data()

        # if loaded successfully
        if self._model.loaded_runs:
            current_run = self._model.loaded_runs[0]
            self._view.set_run_edit_without_validator(str(current_run) + " (CURRENT RUN)")
            self._model.current_run = current_run
        self._view.notify_loading_finished()
        self.enable_loading()

    # ------------------------------------------------------------------------------------------------------------------
    # Loading from increment/decrement run buttons
    # ------------------------------------------------------------------------------------------------------------------

    def handle_increment_run(self):
        run_list = self.get_incremented_run_list()
        if not run_list:
            return
        new_run = max(run_list)

        if self._model.current_run and new_run > self._model.current_run:
            self._view.warning_popup("Requested run exceeds the current run for this instrument")
            return

        file_name = file_utils.file_path_for_instrument_and_run(self.get_current_instrument(), new_run)
        self.handle_loading([file_name], self._use_threading)

    def handle_decrement_run(self):
        run_list = self.get_decremented_run_list()
        if not run_list:
            return
        new_run = min(run_list)

        file_name = file_utils.file_path_for_instrument_and_run(self.get_current_instrument(), new_run)
        self.handle_loading([file_name], self._use_threading)

    def get_incremented_run_list(self):
        """
        Updates list of runs by adding a run equal to 1 after to the highest run.
        """
        run_list = load_utils.flatten_run_list(copy.copy(self.runs))
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_list[0], run_utils.increment_run(run_list[0])]
        else:
            run_list = run_utils.increment_run_list(run_list)
        return run_list

    def get_decremented_run_list(self):
        """
        Updates list of runs by adding a run equal to 1 before to the lowest run.
        """
        run_list = load_utils.flatten_run_list(copy.copy(self.runs))
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_utils.decrement_run(run_list[0]), run_list[0]]
        else:
            run_list = run_utils.decrement_run_list(run_list)
        return run_list
