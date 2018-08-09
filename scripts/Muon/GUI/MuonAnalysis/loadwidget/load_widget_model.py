from __future__ import (absolute_import, division, print_function)


class LoadWidgetModel(object):
    """
    The model is responsible for storing the currently loaded run or runs
    (both the nun numbers, filenames and workspaces) as well as loading new runs using a separate loading thread.
    """

    def __init__(self):
        # list of currently loaded filenames, runs and workspaces (in order)
        self._loaded_filenames = []
        self._loaded_workspaces = []
        self._loaded_runs = []

    def add_muon_data(self, filename, workspace, run):
        self._loaded_filenames += [filename]
        self._loaded_workspaces += [workspace]
        self._loaded_runs += [run]

    def clear_data(self):
        self._loaded_filenames = []
        self._loaded_workspaces = []
        self._loaded_runs = []

    def is_filename_loaded(self, filename):
        return filename in self._loaded_filenames

    def is_run_loaded(self, run):
        return run in self._loaded_runs

    @property
    def workspaces(self):
        return self._loaded_workspaces

    @property
    def runs(self):
        return self._loaded_runs

    @property
    def filenames(self):
        return self._loaded_filenames
