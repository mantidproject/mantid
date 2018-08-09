from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from mantid.kernel import ConfigService
from mantid import config as cf
from qtpy.QtCore import QThread
import os


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

    @property
    def loaded_workspaces(self):
        return self._loaded_workspaces

    @property
    def loaded_runs(self):
        return self._loaded_runs

    # Used with load thread
    def loadData(self, filenames):
        self._filenames = filenames

    # Used with load thread
    def execute(self):
        #QThread.msleep(1000)
        for file in self._filenames:
            ws = self.load_workspace_from_filename(file)
            if ws:
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
        self._loaded_runs = []

    def add_directories_to_config_service(self, file_list):
        print(file_list)
        dirs = [os.path.dirname(filename) for filename in file_list]
        dirs = [filename if os.path.isdir(filename) else "" for filename in dirs]
        dirs = list(set(dirs))
        if dirs:
            for dir in dirs:
                ConfigService.Instance().appendDataSearchDir(dir)

        print("Dirs : ", dirs)
        print("Data search : ", ConfigService.Instance().getDataSearchDirs())
        print("config : ", cf.getDataSearchDirs())
        print(cf["datasearch.directories"])
