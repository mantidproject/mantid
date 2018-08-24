from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.threading_manager as thread_manager


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self, loaded_data_store=MuonLoadData()):
        # Used with load thread
        self._filenames = []

        self._loaded_data_store = loaded_data_store
        self._current_run = None

        self.thread_manager = None

    # TODO : remove the need for this function
    def remove_previous_data(self):
        self._loaded_data_store.remove_last_added_data()

    # Used with load thread
    def loadData(self, filenames):
        self._filenames = filenames

    # Used with load thread
    def execute(self):
        failed_files = []
        for filename in self._filenames:
            try:
                ws, filename, run = self.load_workspace_from_filename(filename)
            except Exception:
                failed_files += [filename]
                continue
            self._loaded_data_store.add_data(run=run, workspace=ws, filename=filename)
        if failed_files:
            message = self.exception_message_for_failed_files(failed_files)
            raise ValueError(message)

    def add_thread_data(self):
        for res in self.thread_manager.results["results"]:
            self._loaded_data_store.add_data(run=res[2], workspace=res[0], filename=res[1])

    def load_with_multithreading(self, filenames, callback_finished, callback_progress, callback_exception):
        self.load_func = thread_manager.threading_decorator(self.load_workspace_from_filename)
        n_threads = min(2, len(filenames))
        self.thread_manager = thread_manager.WorkerManager(fn=self.load_workspace_from_filename, num_threads=n_threads,
                                                           callback_on_progress_update=callback_progress,
                                                           callback_on_thread_exception=callback_exception,
                                                           callback_on_threads_complete=callback_finished,
                                                           filename=filenames)
        self.thread_manager.start()

    def exception_message_for_failed_files(self, failed_file_list):
        return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)

    # Used with load thread
    def output(self):
        pass

    # Used with load thread
    def cancel(self):
        pass

    def load_workspace_from_filename(self, filename):

        alg = mantid.AlgorithmManager.create("LoadMuonNexus")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")

        try:
            alg.setProperty("Filename", filename)
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value
        except Exception:
            # let Load search for the file
            alg.setProperty("Filename", filename.split("\\")[-1])
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value
            filename = alg.getProperty("Filename").value

        run = int(workspace.getRunNumber())

        return workspace, filename, run

    def get_run_list(self):
        return self.loaded_runs

    def clear_loaded_data(self):
        self._loaded_data_store.clear()

    @property
    def current_run(self):
        return self._current_run

    @current_run.setter
    def current_run(self, run):
        self._current_run = run

    @property
    def loaded_filenames(self):
        return self._loaded_data_store.get_parameter("filename")

    @property
    def loaded_workspaces(self):
        return self._loaded_data_store.get_parameter("workspace")

    @property
    def loaded_runs(self):
        return self._loaded_data_store.get_parameter("run")
