# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.utilities.load_utils as load_utils


class LoadRunWidgetModel(object):
    """Stores info on all currently loaded workspaces"""

    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        # Used with load thread
        self._filenames = []

        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context
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
                ws, run, filename, _, period_info = load_utils.load_workspace_from_filename(filename)
                self._data_context.periods_info = period_info
            except ValueError as error:
                failed_files += [(filename, error)]
                continue
            self._loaded_data_store.remove_data(run=[run])
            self._loaded_data_store.add_data(run=[run], workspace=ws, filename=filename,
                                             instrument=self._data_context.instrument)
        if failed_files:
            message = "The requested run could not be found. This could be due to: \n - The run does not yet exist." \
                      "\n - The file was not found locally (please check the user directories)."
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

    @property
    def instrument(self):
        return self._data_context.instrument

    @property
    def current_runs(self):
        return self._data_context.current_runs

    @current_runs.setter
    def current_runs(self, value):
        self._data_context.current_runs = value

    def get_latest_loaded_run(self):
        return self._loaded_data_store.get_latest_data()['run']

    def get_data(self, run):
        return self._loaded_data_store.get_data(run=run, instrument=self._data_context.instrument)
