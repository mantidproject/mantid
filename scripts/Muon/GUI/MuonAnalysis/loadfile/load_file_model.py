from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from qtpy.QtCore import QThread


class BrowseFileWidgetModel(object):

    def __init__(self):
        # Temporary list of filenames used for load thread
        self._filenames = []

        self._loading = False
        self._loaded_filenames = []
        self._loaded_workspaces = []

        self._loaded_runs = []
        self._current_run = None

    @property
    def loaded_filenames(self):
        return self._loaded_filenames

    # Used with load thread
    def loadData(self, filenames):
        self._filenames = filenames

    # Used with load thread
    def execute(self):
        QThread.msleep(1000)
        for file in self._filenames:
            ws = self.load_workspace_from_filename(file)
            self._loaded_workspaces += [ws]
            # TODO : handle exception (not loading data)
            self._loaded_runs += [int(ws[0].getRunNumber())]
            self._loaded_filenames += [file]

    # Used with load thread
    def output(self):
        pass

    # Used with load thread
    def cancel(self):
        pass

    def load_workspace_from_filename(self, filename):

        if self._loading:
            raise RuntimeError("Loading already in progress")

        self._loading = True
        print("Loading file : ", filename)
        workspace = mantid.Load(Filename=filename)
        self._loading = False

        return workspace

    def clear(self):
        self._loading = False
        self._loaded_filenames = []
        self._loaded_workspaces = []
