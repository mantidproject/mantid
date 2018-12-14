# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.utilities.load_utils as load_utils


def exception_message_for_failed_files(failed_file_list):
    return "Could not load the following files : \n - " + "\n - ".join(failed_file_list)


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self, loaded_data_store=MuonLoadData()):
        # Used with load thread
        self._filenames = []

        self._loaded_data_store = loaded_data_store
        self._current_run = None

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

    # This is needed to work with thread model
    def output(self):
        pass

    def cancel(self):
        pass

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
