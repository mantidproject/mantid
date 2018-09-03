from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.run_string_utils as run_utils
import Muon.GUI.Common.muon_file_utils as file_utils

import copy


class LoadRunWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._load_thread = None

        self._load_multiple_runs = False

        # TODO : Replace lines below with code to get the instrument
        self._instrument = "EMU"
        self._view.set_current_instrument(self._instrument)

        self._set_connections()

    def _set_connections(self):
        self._view.on_load_current_run_clicked(self.handle_load_current_run)
        self._view.on_increment_run_clicked(self.handle_increment_run)
        self._view.on_decrement_run_clicked(self.handle_decrement_run)
        self._view.on_run_edit_changed(self.handle_run_changed_by_user)

    def show(self):
        self._view.show()

    def get_current_instrument(self):
        return self._instrument

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear_loaded_data()

    def handle_run_changed_by_user(self):
        run_string = self._view.get_run_edit_text()
        run_list = run_utils.run_string_to_list(run_string)
        file_names = [file_utils.file_path_for_instrument_and_run(self.get_current_instrument(), new_run)
                      for new_run in run_list]

        if len(file_names) > 1 and not self._load_multiple_runs:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_run_edit_from_cache()
            return

        self.handle_load_thread_start(file_names)

    def handle_load_thread_start(self, filenames):
        self._view.notify_loading_started()
        self.disable_loading()
        self._model.load_with_multithreading(filenames, self.handle_load_thread_finished,
                                             self.handle_load_thread_progress,
                                             self.handle_load_thread_exception)

    def handle_load_thread_progress(self, progress):
        pass

    def handle_load_thread_exception(self, **kwargs):
        self._view.warning_popup("")

    def handle_load_thread_finished(self):
        self._model.add_thread_data()
        self._model.thread_manager.clear()

        # If in single file mode, remove the previous run
        if not self._load_multiple_runs and len(self._model.get_run_list()) > 1:
            self._model.remove_previous_data()

        run_list = self._model.get_run_list()
        self.set_run_edit_from_list(run_list)

        self._view.notify_loading_finished()
        self.enable_loading()

    def handle_load_current_run(self):

        try:
            current_run_file = file_utils.get_current_run_filename(self.get_current_instrument())
        except ValueError as e:
            self._view.warning_popup(e.args[0])
            return

        if current_run_file == "":
            self._view.warning_popup("Cannot find directory for current instrument : " + self._instrument)
            return

        self._view.notify_loading_started()
        self.disable_loading()

        self._model.load_with_multithreading([current_run_file], self.handle_load_thread_finished_current_run,
                                             self.handle_load_thread_progress,
                                             self.handle_load_thread_exception)

    def handle_load_thread_finished_current_run(self):

        self._model.add_thread_data()
        self._model.thread_manager.clear()

        # If in single file mode, remove the previous run
        if not self._load_multiple_runs and len(self._model.get_run_list()) > 1:
            self._model.remove_previous_data()

        # if loaded successfully
        if self._model.loaded_runs:
            current_run = self._model.get_run_list()[0]
            self.set_run_edit_from_list([current_run])
            self._model.current_run = current_run

        self._view.notify_loading_finished()
        self.enable_loading()

    def handle_increment_run(self):
        run_list = self.get_incremented_run_list()
        if not run_list:
            return
        new_run = max(run_list)

        if self._model.current_run and new_run > self._model.current_run:
            self._view.warning_popup("Requested run exceeds the current run for this instrument")
            return

        file_name = file_utils.file_path_for_instrument_and_run(self.get_current_instrument(), new_run)
        self.handle_load_thread_start([file_name])

    def handle_decrement_run(self):
        run_list = self.get_decremented_run_list()
        if not run_list:
            return
        new_run = min(run_list)

        instr = self.get_current_instrument()
        file_name = file_utils.file_path_for_instrument_and_run(instr, new_run)
        self.handle_load_thread_start([file_name])

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def set_run_edit_from_list(self, run_list):
        run_string = run_utils.run_list_to_string(run_list)
        self._view.set_run_edit_text(run_string)

    def get_loaded_runs(self):
        return self._model.loaded_runs

    def get_incremented_run_list(self):
        run_list = copy.copy(self.runs)
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_list[0], run_utils.increment_run(run_list[0])]
        else:
            run_list = run_utils.increment_run_list(run_list)
        return run_list

    def get_decremented_run_list(self):
        run_list = copy.copy(self.runs)
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_utils.decrement_run(run_list[0]), run_list[0]]
        else:
            run_list = run_utils.decrement_run_list(run_list)
        return run_list

    def set_new_instrument(self):
        # TODO : implement
        pass

    # def clear_loaded_data(self):
    #     self._view.set_run_edit_text("")
    #     self._model.clear_loaded_data()

    def enable_multiple_files(self, enabled):
        if enabled:
            print("Enabling multiple files (run widget)")
        else:
            print("Disabling multiple files (run widget)")
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
