from __future__ import (absolute_import, division, print_function)

import os
import time

import mantid.simpleapi as mantid
from mantid.kernel import ConfigService

from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.threading_manager as thread_manager


def exception_message_for_failed_files(failed_file_list):
    return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)

def empty_function(**kwargs):
    return


class BrowseFileWidgetModel(object):

    def __init__(self, loaded_data_store=MuonLoadData()):

        self._loaded_data_store = loaded_data_store

        self.thread_manager = None

    @property
    def loaded_filenames(self):
        return self._loaded_data_store.get_parameter("filename")

    @property
    def loaded_workspaces(self):
        return self._loaded_data_store.get_parameter("workspace")

    @property
    def loaded_runs(self):
        return self._loaded_data_store.get_parameter("run")

    def clear(self):
        self._loaded_data_store.clear()

    def cancel_threads(self):
        if self.thread_manager:
            self.thread_manager.cancel()

    def load_with_multithreading(self, filenames,
                                 callback_finished=lambda: 0, callback_progress=lambda dbl: 0,
                                 callback_exception=empty_function, callback_cancelled=lambda: 0):
        # TODO : add sensible thread number
        n_threads = min(4, max(len(filenames), 1))
        # n_threads = min(10,len(filenames))
        if self.thread_manager:
            self.thread_manager.clear()
            self.thread_manager.deleteLater()
            self.thread_manager = None
        self.thread_manager = thread_manager.WorkerManager(fn=self.load_workspace_from_filename, num_threads=n_threads,
                                                           callback_on_progress_update=callback_progress,
                                                           callback_on_thread_exception=callback_exception,
                                                           callback_on_threads_complete=callback_finished,
                                                           callback_on_threads_cancelled=callback_cancelled,
                                                           filename=filenames)
        self.thread_manager.start()

    def load_workspace_from_filename(self, filename):

        alg = mantid.AlgorithmManager.create("Load")
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

        return filename, workspace, run

    def add_thread_data(self):
        # If data already loaded in model, remove it in favour of the most recent load
        for res in self.thread_manager.results["results"]:
            if self._loaded_data_store.contains(run=res[2]):
                self._loaded_data_store.remove_data(run=res[2], workspace=res[1], filename=res[0])
            self._loaded_data_store.add_data(run=res[2], workspace=res[1], filename=res[0])

    def add_directories_to_config_service(self, file_list):
        dirs = [os.path.dirname(filename) for filename in file_list]
        dirs = [filename if os.path.isdir(filename) else "" for filename in dirs]
        dirs = list(set(dirs))
        if dirs:
            for new_dir in dirs:
                ConfigService.Instance().appendDataSearchDir(new_dir.encode('ascii', 'ignore'))
