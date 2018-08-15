from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_load_data import MuonLoadData

class LoadWidgetModel(object):
    """
    The model is responsible for storing the currently loaded run or runs
    (both the nun numbers, filenames and workspaces) as well as loading new runs using a separate loading thread.
    """

    def __init__(self, loaded_data_store = MuonLoadData()):
        self._loaded_data_store = loaded_data_store

    def add_muon_data(self, filename, workspace, run):
        self._loaded_data_store.add_data(run=run, filename=filename, workspace=workspace)

    def clear_data(self):
        self._loaded_data_store.clear()

    def is_filename_loaded(self, filename):
        return self._loaded_data_store.contains(filename=filename)

    def is_run_loaded(self, run):
        return self._loaded_data_store.contains(run=run)

    @property
    def workspaces(self):
        return self._loaded_data_store.get_parameter("workspace")

    @property
    def runs(self):
        return self._loaded_data_store.get_parameter("run")

    @property
    def filenames(self):
        return self._loaded_data_store.get_parameter("filename")
