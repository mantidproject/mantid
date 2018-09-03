from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid
from mantid.kernel import ConfigService
from mantid.api import WorkspaceGroup

from Muon.GUI.Common.muon_load_data import MuonLoadData


def is_workspace_group(workspace):
    return isinstance(workspace, WorkspaceGroup)


def exception_message_for_failed_files(failed_file_list):
    return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)


def get_run_from_multi_period_data(workspace_list):
    """Checks if multi-period data has a single consistent run number and returns it, otherwise raises ValueError."""
    runs = [ws.getRunNumber() for ws in workspace_list]
    unique_runs = list(set(runs))
    if len(unique_runs) != 1:
        raise ValueError("Multi-period data contains >1 unique run number.")
    else:
        return unique_runs[0]


class BrowseFileWidgetModel(object):

    def __init__(self, loaded_data_store=MuonLoadData()):
        # Temporary list of filenames used for load thread
        self._filenames = []

        self._loaded_data_store = loaded_data_store

    @property
    def loaded_filenames(self):
        return self._loaded_data_store.get_parameter("filename")

    @property
    def loaded_workspaces(self):
        return self._loaded_data_store.get_parameter("workspace")

    @property
    def loaded_runs(self):
        return self._loaded_data_store.get_parameter("run")

    # Used with load thread
    def output(self):
        pass

    # Used with load thread
    def cancel(self):
        pass

    # Used with load thread
    def loadData(self, filename_list):
        self._filenames = filename_list

    # Used with load thread
    def execute(self):
        failed_files = []
        for filename in self._filenames:
            try:
                ws, run = self.load_workspace_from_filename(filename)
            except Exception:
                failed_files += [filename]
                continue
            self._loaded_data_store.remove_data(run=run)
            self._loaded_data_store.add_data(run=run, workspace=ws, filename=filename)
        if failed_files:
            message = exception_message_for_failed_files(failed_files)
            raise ValueError(message)

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
            alg.setProperty("Filename", filename.split(os.sep)[-1])
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value

        # handle multi-period data
        if is_workspace_group(workspace):
            workspaces = [ws for ws in workspace]
            run = self.get_run_from_multi_period_data(workspaces)
        else:
            workspaces = [workspace]
            run = int(workspace.getRunNumber())

        return workspaces, run

    def clear(self):
        self._loaded_data_store.clear()

    def add_directories_to_config_service(self, file_list):
        """
        Parses file_list into the unique directories containing the files, and adds these
        to the global config service. These directories will then be automatically searched in
        all subsequent Load calls.
        """
        dirs = [os.path.dirname(filename) for filename in file_list]
        dirs = [filename if os.path.isdir(filename) else "" for filename in dirs]
        dirs = list(set(dirs))
        if dirs:
            for directory in dirs:
                ConfigService.Instance().appendDataSearchDir(directory.encode('ascii', 'ignore'))
