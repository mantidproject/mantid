from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from qtpy.QtCore import QThread


class LoadRunWidgetModel(object):

    def __init__(self):
        self._loading = False
        self._workspace = None
        self._filenames = None
        self._loaded_runs = None
        self._current_run = None

    def loadData(self, filenames):
        self._filenames = filenames

    def execute(self):
        QThread.msleep(5000)
        self._workspace = self.load_workspace_from_filename(self._filenames)

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

    @property
    def current_run(self):
        return self._current_run

    @current_run.setter
    def current_run(self, run):
        self._current_run = run
