# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
from mantidqtinterfaces.Muon.GUI.Common import thread_model
import mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils as load_utils
import mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils as run_utils
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import flatten_run_list
import mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_utils_ea as load_utils_ea
from mantidqt.utils.observer_pattern import GenericObservable, Observable


class LoadRunWidgetPresenterEA(object):
    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._load_thread = None

        self._load_multiple_runs = True
        self._multiple_run_mode = "Simultaneous"

        self._view.set_current_instrument(self._model.instrument)

        self.run_list = []

        self._set_connections()
        self.enable_notifier = self.EnableEditingNotifier(self)
        self.disable_notifier = self.DisableEditingNotifier(self)

        self.updated_directory = GenericObservable()

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

    def set_current_instrument(self, instrument):
        self._view.set_current_instrument(instrument)

    def disable_loading(self):
        self._view.disable_load_buttons()
        self.thread_success = True

    def enable_loading(self):
        self._view.enable_load_buttons()

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear_loaded_data()

    def clear_data(self):
        self._model.clear_data()

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def runs(self):
        return self._model.loaded_runs

    # used by parent widget
    def update_view_from_model(self, run_list):
        self.set_run_edit_from_list(run_list)

    def update_multiple_loading_behaviour(self, multiple_run_mode):
        self._multiple_run_mode = multiple_run_mode

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
        if not run_string:
            return

        try:
            self.run_list = run_utils.run_string_to_list(run_string)
        except IndexError as err:
            self._view.warning_popup(err.args[0])
            return

        if run_string:
            self.load_runs(self.run_list)
        else:
            self.on_loading_finished()

    # ------------------------------------------------------------------------------------------------------------------
    # Loading from current run button
    # ------------------------------------------------------------------------------------------------------------------

    def handle_load_current_run(self):
        self._view.warning_popup("Load Current Run is currently not available")

    # ------------------------------------------------------------------------------------------------------------------
    # Loading from increment/decrement run buttons
    # ------------------------------------------------------------------------------------------------------------------

    def handle_increment_run(self):
        incremented_run_list = self.get_incremented_run_list()
        self.run_list = [max(incremented_run_list)] if incremented_run_list else []
        if not self.run_list:
            return
        new_run = max(self.run_list)

        self.load_runs([new_run])

    def handle_decrement_run(self):
        decremented_run_list = self.get_decremented_run_list()
        self.run_list = [min(decremented_run_list)] if decremented_run_list else []
        if not self.run_list:
            return
        new_run = min(self.run_list)

        self.load_runs([new_run])

    def get_incremented_run_list(self):
        """
        Updates list of runs by adding a run equal to 1 after the highest run.
        """
        run_list = load_utils.flatten_run_list(copy.copy(self._model.current_runs))
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_list[0], run_utils.increment_run(run_list[0])]
        else:
            run_list = run_utils.increment_run_list(run_list)
        return run_list

    def get_decremented_run_list(self):
        """
        Updates list of runs by adding a run equal to 1 before the lowest run.
        """
        run_list = load_utils.flatten_run_list(copy.copy(self._model.current_runs))
        if run_list is None or len(run_list) == 0:
            return []
        if len(run_list) == 1:
            run_list = [run_utils.decrement_run(run_list[0]), run_list[0]]
        else:
            run_list = run_utils.decrement_run_list(run_list)
        return run_list

    def load_runs(self, runs):
        self._model._data_context.previous_runs = []
        self._model._data_context.previous_runs = copy.copy(self._model._data_context.current_runs)
        self._model._data_context.current_runs = []
        self.disable_notifier.notify_subscribers()
        self.handle_loading(runs)

    def handle_loading(self, runs):
        self.handle_load_thread_start(runs, self.handle_load_thread_finished)

    def on_loading_start(self):
        self._view.notify_loading_started()
        self.disable_loading()

    def handle_load_thread_start(self, runs, finished_callback):
        self.on_loading_start()
        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading, finished_callback, self.error_callback)
        self._load_thread.loadData(runs)
        self._load_thread.start()

    def error_callback(self, error_message):
        self.thread_success = False
        self.enable_notifier.notify_subscribers()
        self._view.warning_popup(error_message)

    def handle_load_thread_finished(self):
        if self._load_thread is not None:
            self._load_thread.deleteLater()
        self._load_thread = None

        if not self.thread_success:
            self.run_list = flatten_run_list(self._model.current_runs)
        self.on_loading_finished()

    def on_loading_finished(self):
        try:
            if not self._model._data_context.current_runs:
                self._model._data_context.current_runs = copy.copy(self._model._data_context.previous_runs)
            if self._load_multiple_runs and self._multiple_run_mode == "Co-Add":
                if len(self._model._data_context.current_runs) > 1:
                    load_utils_ea.combine_loaded_runs(self._model, self._model._data_context.current_runs)

            self.update_view_from_model(self._model._data_context.current_runs)
            self.updated_directory.notify_subscribers(self._model._directory)
            self._view.notify_loading_finished()
        except ValueError:
            self._view.warning_popup("Attempting to co-add data with different time bins. This is not currently supported.")
            self._view.reset_run_edit_from_cache()
        except RuntimeError as error:
            self._view.warning_popup(error)
            self._view.reset_run_edit_from_cache()
        finally:
            self.enable_notifier.notify_subscribers()

    def handle_updated_path_text(self):
        """
        Handles the current directory path being observed
        """
        path = self._model._directory
        return path

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
