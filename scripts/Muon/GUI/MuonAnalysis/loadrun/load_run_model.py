from __future__ import (absolute_import, division, print_function)

from mantid.api import WorkspaceGroup

from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.load_utils as load_utils


def is_workspace_group(workspace):
    return isinstance(workspace, WorkspaceGroup)


def get_run_from_multi_period_data(workspace_list):
    """
    Checks if multi-period data has a single consistent
    run number and returns it, otherwise raises ValueError.
    """
    runs = [ws.getRunNumber() for ws in workspace_list]
    unique_runs = list(set(runs))
    if len(unique_runs) != 1:
        raise ValueError("Multi-period data contains >1 unique run number.")
    else:
        return unique_runs[0]


def exception_message_for_failed_files(failed_file_list):
    return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self, loaded_data_store=MuonLoadData()):
        # Used with load thread
        self._filenames = []

        self._loaded_data_store = loaded_data_store
        self._current_run = None

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
                ws, run, filename = load_utils.load_workspace_from_filename(filename)
            except Exception:
                failed_files += [filename]
                continue
            self._loaded_data_store.remove_data(run=run)
            self._loaded_data_store.add_data(run=run, workspace=ws, filename=filename)
        if failed_files:
            message = exception_message_for_failed_files(failed_files)
            raise ValueError(message)

    # Used with load thread
    def output(self):
        pass

    # Used with load thread
    def cancel(self):
        pass

    def get_instrument(self):
        instrument = "None"
        try:
            workspace = self.loaded_workspaces[0]["OutputWorkspace"]
            if isinstance(workspace, list):
                instrument = workspace[0].workspace.getInstrument().getName()
            else:
                instrument = workspace.workspace.getInstrument().getName()
        except IndexError:
            pass
        return instrument

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
