from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from qtpy.QtCore import QThread


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self):
        self._loading = False

        self._filenames = None

        self._loaded_workspaces = []
        self._loaded_filenames = []
        self._loaded_runs = []
        self._current_run = None

    def loadData(self, filenames):
        self._filenames = filenames

    def execute(self):
        #QThread.msleep(1000)
        for file in self._filenames:
            ws, filename = self.load_workspace_from_filename(file)
            self._loaded_workspaces += [ws]
            self._loaded_filenames += [filename]
            # TODO : Add checks for whether workspace is valid
            self._loaded_runs += [int(ws.getRunNumber())]

    def output(self):
        pass

    def cancel(self):
        pass

    def load_workspace_from_filename(self, filename):
        if self._loading:
            raise RuntimeError("Loading already in progress")
        self._loading = True

        alg = mantid.AlgorithmManager.create("LoadMuonNexus")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")

        try:
            alg.setProperty("Filename", filename)
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value
        except:
            # let Load search for the file
            alg.setProperty("Filename", filename.split("\\")[-1])
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value
            filename = alg.getProperty("Filename").value

        print("Loaded file successfully from : " + filename)

        self._loading = False

        return workspace, filename

    def set_loaded_runs(self, run_list):
        self._loaded_runs = run_list

    def get_run_list(self):
        return self._loaded_runs

    def clear_loaded_data(self):
        self._loading = False
        self._loaded_workspaces = []
        self._loaded_filenames = []
        self._loaded_runs = []

    @property
    def current_run(self):
        return self._current_run

    @current_run.setter
    def current_run(self, run):
        self._current_run = run

    @property
    def loaded_filenames(self):
        return self._loaded_filenames

    @property
    def loaded_workspaces(self):
        return self._loaded_workspaces

    @property
    def loaded_runs(self):
        return self._loaded_runs