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

        self._loaded_filenames = []
        self._loaded_workspaces = []
        self._loaded_runs = []

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
    def loadData(self, filename_list):
        self._filenames = filename_list

    # Used with load thread
    def execute(self):
        failed_files = []
        for filename in self._filenames:
            try:
                ws, run = self.load_workspace_from_filename(filename)
            except ValueError:
                failed_files += [filename]
                continue
            self._loaded_workspaces += [ws]
            self._loaded_runs += [run]
            self._loaded_filenames += [filename]
        if failed_files:
            message = self.exception_message_for_failed_files(failed_files)
            raise ValueError(message)

    def exception_message_for_failed_files(self, failed_file_list):
        print("Exception in execute!")
        return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)

    # Used with load thread
    def output(self):
        pass

    # Used with load thread
    def cancel(self):
        pass

    def load_workspace_from_filename(self, filename):
        print("Loading file : ", filename)
        try:
            workspace = mantid.Load(Filename=filename)
            run = int(workspace[0].getRunNumber())
        except ValueError as e:
            raise ValueError(e.args)
        return workspace, run

    def clear(self):
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
                ConfigService.Instance().appendDataSearchDir(dir.encode('ascii', 'ignore'))

        print("Dirs : ", dirs)
        print("Data search : ", ConfigService.Instance().getDataSearchDirs())
        print("config : ", cf.getDataSearchDirs())
        print(cf["datasearch.directories"])
