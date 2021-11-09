# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadElementalAnalysisData
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.context.data_context import RunObject


class LoadWidgetModel(object):
    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context
        self._context = context
        return

    @property
    def runs(self):
        return self._data_context.current_runs

    @property
    def current_runs(self):
        return self._data_context.current_runs

    @current_runs.setter
    def current_runs(self, value):
        self._data_context.current_runs = value

    def clear_data(self):
        self._loaded_data_store.clear()
        self._data_context.current_runs = []

    def update_current_data(self):
        self._context.update_current_data()


class BrowseFileWidgetModel(object):
    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context

    @property
    def loaded_runs(self):
        return self._loaded_data_store.get_parameter("run")

    @property
    def current_runs(self):
        return self._data_context.current_runs

    @current_runs.setter
    def current_runs(self, value):
        self._data_context.current_runs = value

    def clear(self):
        self._loaded_data_store.clear()


class LoadRunWidgetModel(object):
    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context
        self._runs = []
        self._current_run = None
        self._directory = ""

    # Used with load thread
    def loadData(self, runs):
        self._runs = runs

    # Used with load thread
    def execute(self):
        failed_files = []
        for run in self._runs:
            if self._loaded_data_store.get_data(run=[run]):
                self._data_context.current_runs.append(run)
            else:
                try:
                    groupws, self._directory = LoadElementalAnalysisData(Run=run, GroupWorkspace=str(run))
                    detectors = []
                    workspace_list = groupws.getNames()
                    for item in workspace_list:
                        ''' The workspaces in the groupworkspace are all named in the format [run];[detector]
                        The line below removes any text up to and including the ; which leaves behind the detector name.
                        For example 2695; Detector 1 returns as Detector 1'''
                        detector_name = item.split(';', 1)[-1].lstrip()
                        detectors.append(detector_name)
                    run_results = RunObject(run, detectors, groupws)
                    self._data_context.run_info_update(run_results)
                    self._data_context.current_runs.append(run)

                except RuntimeError as error:
                    failed_files += [(run, error)]
                    continue
                self._data_context._loaded_data.add_data(run=[run], workspace=groupws)

        if failed_files:
            message = "The requested run could not be found. This could be due to: \n - The run does not yet exist." \
                      "\n - The file was not found locally (please check the user directories)."
            raise ValueError(message)

    def cancel(self):
        pass

    def clear_loaded_data(self):
        self._loaded_data_store.clear()

    @property
    def current_runs(self):
        return self._data_context.current_runs

    @current_runs.setter
    def current_runs(self, value):
        self._data_context.current_runs = value

    @property
    def instrument(self):
        return self._data_context.instrument

    @property
    def loaded_runs(self):
        return self._loaded_data_store.get_parameter("run")

    def get_latest_loaded_run(self):
        return self._loaded_data_store.get_latest_data()['run']

    def get_data(self, run):
        return self._loaded_data_store.get_data(run=run, instrument=self._data_context.instrument)
