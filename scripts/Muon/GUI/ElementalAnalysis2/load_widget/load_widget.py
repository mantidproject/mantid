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


class LoadWidgetModel(object):
    def __init__(self, context=None):
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

    @property
    def filenames(self):
        return
        # return self._data_context.current_filenames


class BrowseFileWidgetModel(object):
    def __init__(self):
        return


class LoadRunWidgetModel(object):
    def __init__(self, loaded_data_store=MuonLoadData(), context=None):
        self._loaded_data_store = loaded_data_store
        self._data_context = context.data_context
        self._runs = []
        self._current_run = None

    # Used with load thread
    def loadData(self, runs):
        self._runs = runs

    # Used with load thread
    def execute(self):
        failed_files = []
        for run in self._runs:
            try:
                LoadElementalAnalysisData(Run=run, GroupWorkspace=str(run))
            except ValueError as error:
                failed_files += [(run, error)]
                continue
                # self._loaded_data_store.remove_data(run=[run])
                # self._loaded_data_store.add_data(run=[run], workspace=ws, filename=filename,
                #                                  instrument=self._data_context.instrument)
        if failed_files:
            message = "The requested run could not be found. This could be due to: \n - The run does not yet exist." \
                      "\n - The file was not found locally (please check the user directories)."
            raise ValueError(message)

        # This is needed to work with thread model

    def output(self):
        pass

    def cancel(self):
        pass

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
        return
        #return self._loaded_data_store.get_parameter("run")


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
                                               LoadWidgetModel(context))

        self.file_widget = BrowseFileWidgetPresenter(self.load_file_view, BrowseFileWidgetModel())
        self.run_widget = LoadRunWidgetPresenterEA(self.load_run_view, LoadRunWidgetModel(loaded_data, context))

        self.load_widget.set_load_file_widget(self.file_widget)
        self.load_widget.set_load_run_widget(self.run_widget)

        self.load_widget.set_current_instrument(context.data_context.instrument)

        #context.update_view_from_model_notifier.add_subscriber(self.load_widget.update_view_from_model_observer)

    @property
    def view(self):
        return self.load_widget_view