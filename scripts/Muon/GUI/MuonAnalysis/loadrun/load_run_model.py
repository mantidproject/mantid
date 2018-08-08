from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from qtpy.QtCore import QThread


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self):
        self._loading = False

        self._filenames = None

        self._loaded_workspaces = []
        self._loaded_filenames = None
        self._loaded_runs = []
        self._current_run = None

    def loadData(self, filenames):
        self._filenames = filenames

    def execute(self):
        QThread.msleep(1000)
        for file in self._filenames:
            ws = self.load_workspace_from_filename(file)
            self._loaded_workspaces += [ws]
            self._loaded_runs += [int(ws.getRunNumber())]

    def output(self):
        pass

    def cancel(self):
        pass

    def load_workspace_from_filename(self, filename):
        if self._loading:
            raise RuntimeError("Loading already in progress")
        self._loading = True
        outWS = mantid.LoadMuonNexus(Filename=filename)
        self._loading = False
        return outWS[0]

    def set_loaded_runs(self, run_list):
        self._loaded_runs = run_list

    def get_run_list(self):
        return self._loaded_runs

    def clear_loaded_data(self):
        self._loading = False
        self._loaded_workspaces = []
        self._loaded_filenames = None
        self._loaded_runs = []

    @property
    def current_run(self):
        return self._current_run

    @current_run.setter
    def current_run(self, run):
        self._current_run = run
