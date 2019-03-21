# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_load_data import MuonLoadData


class LoadWidgetModel(object):
    """
    The model is responsible for storing the currently loaded run or runs
    (both the nun numbers, filenames and workspaces) as well as loading new runs using a separate loading thread.
    """

    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context

    def add_muon_data(self, filename, workspace, run):
        self._loaded_data_store.add_data(run=run, filename=filename, workspace=workspace)

    def clear_data(self):
        self._loaded_data_store.clear()
        self._data_context.current_runs = []

    def is_filename_loaded(self, filename):
        return self._loaded_data_store.contains(filename=filename)

    def is_run_loaded(self, run):
        return self._loaded_data_store.contains(run=run)

    @property
    def workspaces(self):
        return self._data_context.current_workspaces

    @property
    def runs(self):
        return self._data_context.current_runs

    @property
    def filenames(self):
        return self._data_context.current_filenames
