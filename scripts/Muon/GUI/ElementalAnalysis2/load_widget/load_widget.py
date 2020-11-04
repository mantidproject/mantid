# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadElementalAnalysisData
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.muon_load_data import MuonLoadData

from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA

from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView
from Muon.GUI.ElementalAnalysis2.load_widget.load_widget_presenter import LoadWidgetPresenterEA

from Muon.GUI.ElementalAnalysis2.context.context import RunObject


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
            try:
                groupws, path = LoadElementalAnalysisData(Run=run, GroupWorkspace=str(run))
                self._directory = path
                detectors = []
                workspace_list = groupws.getNames()
                for item in workspace_list:
                    detector_name = item.split(';', 1)[-1].lstrip()
                    detectors.append(detector_name)
                runResults = RunObject(run, detectors, groupws)
                self._data_context.run_info_update(run, runResults)
            except ValueError as error:
                failed_files += [(run, error)]
                continue
            self._loaded_data_store.add_data(run=[run], workspace=groupws)

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


class LoadWidget(object):
    def __init__(self, loaded_data, context, parent):
        # set up the views
        self.load_file_view = BrowseFileWidgetView(parent)
        self.load_file_view.hide_browse()
        self.load_run_view = LoadRunWidgetView(parent)
        self.load_widget_view = LoadWidgetView(parent=parent,
                                               load_file_view=self.load_file_view,
                                               load_run_view=self.load_run_view)
        self.load_widget = LoadWidgetPresenterEA(self.load_widget_view,
                                                 LoadWidgetModel(loaded_data, context))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel(loaded_data, context))
        self.run_widget = LoadRunWidgetPresenterEA(self.load_run_view, LoadRunWidgetModel(loaded_data, context))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

        self.load_widget.set_current_instrument(context.data_context.instrument)

        self.run_widget.updated_directory.add_subscriber(self.file_widget.updated_file_path)

        #context.update_view_from_model_notifier.add_subscriber(self.load_widget.update_view_from_model_observer)

    @property
    def view(self):
        return self.load_widget_view
