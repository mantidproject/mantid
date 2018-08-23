from __future__ import (absolute_import, division, print_function)

import copy

import Muon.GUI.Common.muon_file_utils as file_utils


class BrowseFileWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        # Whether to allow single or multiple files to be loaded
        self._multiple_files = True

        self._view.on_browse_clicked(self.on_browse_button_clicked)
        self._view.on_file_edit_changed(self.handle_file_changed_by_user)

    def show(self):
        self._view.show()

    def get_filenames_from_user(self):
        file_filter = file_utils.filter_for_extensions(["nxs"])
        directory = ""
        filenames = self._view.show_file_browser_and_return_selection(file_filter, [directory],
                                                                      multiple_files=self._multiple_files)
        filenames = self.validate_filenames_selection(filenames)
        return filenames

    def validate_filenames_selection(self, filenames):
        filenames = file_utils.parse_user_input_to_files(";".join(filenames))
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        return filenames

    def on_browse_button_clicked(self):
        filenames = self.get_filenames_from_user()
        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        if filenames:
            self.handle_load_thread_start(filenames)

    def handle_file_changed_by_user(self):
        user_input = self._view.get_file_edit_text()
        filenames = file_utils.parse_user_input_to_files(user_input)
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        if not filenames:
            self._view.reset_edit_to_cached_value()
            return
        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        self.handle_load_thread_start(filenames)

    def handle_load_thread_start(self, filenames):
        self._view.notify_loading_started()
        self.disable_loading()

        if len(filenames) > 10:
            self._view.show_progress_bar(self.handle_cancel_threads)

        self._model.load_with_multithreading(filenames,
                                             self.handle_load_thread_finished,
                                             self.handle_load_thread_progress,
                                             self.handle_load_thread_exception,
                                             self.handle_load_threads_cancelled)

    def handle_load_thread_progress(self, progress):
        self._view.set_progress_bar(progress)

    def handle_load_thread_exception(self, **kwargs):
        self._view.warning_popup(kwargs.get("value", ""))

    def handle_cancel_threads(self):
        print("View requests a cancellation")
        self._model.cancel_threads()
        #print("Handle cancel threads 2")
        #self._model.thread_manager.clear()
        #print("Handle cancel threads 3")
        # self._view.remove_progress_bar()
        # print("Handle cancel threads 4")
        # self._view.notify_loading_finished()
        # self.enable_loading()

    def handle_load_threads_cancelled(self):
        print("handle_load_threads_cancelled()")
        self._model.thread_manager.clear()
        self._view.remove_progress_bar()
        self._view.notify_loading_finished()
        self.enable_loading()

    def handle_load_thread_finished(self):
        print("handle_load_thread_finished()")
        self._model.add_thread_data()
        self._model.thread_manager.clear()
        self.on_loading_finished()

    def on_loading_finished(self):
        file_list = self._model.loaded_filenames
        self._view.remove_progress_bar()
        self.set_file_edit(file_list)
        self._view.notify_loading_finished()
        self.enable_loading()
        self._model.add_directories_to_config_service(file_list)

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear()

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def enable_multiple_files(self, enabled):
        self._multiple_files = enabled

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def runs(self):
        return self._model.loaded_runs

    def set_file_edit(self, file_list):
        display_text = len(file_list) > 10
        file_list = sorted(copy.copy(file_list))
        self._view.set_file_edit(";".join(file_list), display_text)

    # for use by parent widget
    def update_view_from_model(self, file_list):
        self.set_file_edit(file_list)
