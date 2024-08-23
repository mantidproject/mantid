# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy

from mantidqtinterfaces.Muon.GUI.Common import thread_model
import mantidqtinterfaces.Muon.GUI.Common.utilities.muon_file_utils as file_utils
import mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils as load_utils
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing


class BrowseFileWidgetPresenter(object):
    def __init__(self, view, model):
        self._view = view
        self._model = model

        # Whether to allow single or multiple files to be loaded
        self._multiple_files = True
        self._multiple_file_mode = "Simultaneous"

        self._use_threading = True
        self._load_thread = None

        self.filenames = []

        self._view.on_browse_clicked(self.on_browse_button_clicked)
        self._view.on_file_edit_changed(self.handle_file_changed_by_user)

        self.updated_file_path = GenericObserverWithArgPassing(self.set_file_edit_directory)

    def show(self):
        self._view.show()

    def cancel(self):
        if self._load_thread is not None:
            self._load_thread.cancel()

    def create_load_thread(self):
        return thread_model.ThreadModel(self._model)

    def update_multiple_loading_behaviour(self, text):
        self._multiple_file_mode = text

    def get_filenames_from_user(self):
        file_filter = file_utils.filter_for_extensions(file_utils.allowed_extensions)
        directory = ""
        filenames = self._view.show_file_browser_and_return_selection(file_filter, [directory], multiple_files=self._multiple_files)
        # validate
        filenames = file_utils.parse_user_input_to_files(";".join(filenames))
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        return filenames

    def on_browse_button_clicked(self):
        filenames = self.get_filenames_from_user()
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        self.filenames = filenames

        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        if filenames:
            self.handle_loading(filenames)

    def handle_file_changed_by_user(self):
        user_input = self._view.get_file_edit_text()
        filenames = file_utils.parse_user_input_to_files(user_input)
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        self.filenames = filenames

        if not filenames:
            self._view.reset_edit_to_cached_value()
            return
        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        if self._multiple_file_mode == "Co-Add":
            # We don't want to allow messy appending when co-adding
            self.clear_loaded_data()
        self.handle_loading(filenames)

    def handle_loading(self, filenames):
        unloaded_file_names = []
        for filename in filenames:
            if not self._model.get_data(filename=filename):
                unloaded_file_names.append(filename)

        if self._use_threading:
            self.handle_load_thread_start(filenames)
        else:
            self.handle_load_no_threading(filenames)

    def handle_load_no_threading(self, filenames):
        self._view.notify_loading_started()
        self.disable_loading()
        self._model.loadData(filenames)
        try:
            self._model.execute()
        except ValueError as error:
            self._view.warning_popup(str(error))
            self.filenames = self._model.current_filenames
        self.on_loading_finished()

    def handle_load_thread_start(self, filenames):
        if self._load_thread:
            return
        self._view.notify_loading_started()
        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading, self.handle_load_thread_finished, self.handle_load_error)
        self._load_thread.loadData(filenames)
        self._load_thread.start()

    def handle_load_error(self, message):
        self.thread_success = False
        self._view.warning_popup(message)

    def handle_load_thread_finished(self):
        self._load_thread = None

        if not self.thread_success:
            self.filenames = self._model.current_filenames

        self.on_loading_finished()

    def on_loading_finished(self):
        instrument_from_workspace = self._model.get_instrument_from_latest_run()
        if instrument_from_workspace != self._model._data_context.instrument or instrument_from_workspace == "PSI":
            self._model.instrument = instrument_from_workspace

        if self._multiple_files and self._multiple_file_mode == "Co-Add":
            file_list = [
                filename
                for filename in self.filenames
                if self._model.get_data(filename=filename, instrument=self._model._data_context.instrument)
            ]
            run_list_to_add = [self._model.get_data(filename=filename)["run"][0] for filename in file_list]
            run_list = [[self._model.get_data(filename=filename)["run"][0] for filename in file_list]]
            load_utils.combine_loaded_runs(self._model, run_list_to_add)

        else:
            file_list = [
                filename
                for filename in self.filenames
                if self._model._loaded_data_store.get_data(filename=filename, instrument=self._model.instrument)
            ]
            run_list = [self._model.get_data(filename=filename)["run"] for filename in file_list]

        self.set_file_edit(file_list)
        if file_list == []:
            self._view.warning_popup(
                "This data is for a different instrument and has not been loaded."
                "The instrument selection on the home tab has been updated."
                "Please reload your data."
            )
        self._model.current_runs = run_list

        self._view.notify_loading_finished()
        self.enable_loading()
        self._model.add_directories_to_config_service(file_list)

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear()

    def disable_loading(self):
        self.thread_success = True
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def runs(self):
        return self._model.loaded_runs

    def set_file_edit(self, file_list):
        file_list = sorted(copy.copy(file_list))
        if file_list == []:
            self._view.set_file_edit("No data loaded", False)
        else:
            self._view.set_file_edit(";".join(file_list), False)

    # used by parent widget
    def update_view_from_model(self, file_list):
        self.set_file_edit(file_list)

    def set_current_instrument(self, instrument):
        pass

    def set_file_edit_directory(self, directory):
        self._view.set_file_edit(directory, False)
