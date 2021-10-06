# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_presenter import MainPlotWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_presenter import PlotDataPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.model_fit_pane.plot_model_fit_pane_presenter import PlotModelFitPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.model_fit_pane.plot_model_fit_pane_model import PlotModelFitPaneModel
from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_presenter import RawPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model import RawPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_view import RawPaneView


# These are just used internally for ID purposes
# the rest of the code uses the names from the models
DATA = "data"
FIT = "fit"
RAW = "raw"
MODEL = "model"


class MuonAnalysisPlotWidget(object):
    def __init__(self, context=None, parent=None):
        self._parent = parent
        self._context = context
        self._panes = []
        self.modes = {}
        self._create_default_panes()

    def _create_default_panes(self):
        self.view = MainPlotWidgetView(self._parent)
        # The plotting canvas widgets
        self.plotting_canvas_widgets = {}
        # The UI view
        self._views = {}
        # create default panes
        self.create_data_pane()
        self.create_fit_pane()

    def insert_plot_panes(self):
        self.presenter = MainPlotWidgetPresenter(self.view, self._panes)
        self._current_plot_mode = self.presenter.get_plot_mode
        self.presenter.set_plot_mode_changed_slot(self.handle_plot_mode_changed_by_user)
        # add the observers
        for observer in self.presenter.workspace_replaced_in_ads_observers:
            self._context.update_plots_notifier.add_subscriber(observer)
        for observer in self.presenter.workspace_deleted_from_ads_observers:
            self._context.deleted_plots_notifier.add_subscriber(observer)

    @property
    def clear_plot_observers(self):
        return self.presenter.clear_plot_observers

    @property
    def data_changed_observers(self):
        return self.presenter.data_changed_observers

    def close(self):
        self.view.close()

    def set_plot_view(self, plot_mode):
        self.view.set_plot_mode(plot_mode)

    @property
    def rebin_options_set_observers(self):
        return self.presenter.rebin_options_set_observers

    def mode_name(self, name):
        if name in self.modes.keys():
            return self.modes[name].name
        else:
            return None

    def handle_plot_mode_changed_by_user(self):
        old_plot_mode = self._current_plot_mode
        self._current_plot_mode = self.presenter.get_plot_mode
        if old_plot_mode == self._current_plot_mode:
            return

        if old_plot_mode == self.mode_name(DATA) or old_plot_mode == self.mode_name(FIT):
            # make sure the fit and data pane quickedits are sync'd
            pane_to_match = self.mode_name(FIT) if old_plot_mode == self.mode_name(DATA) else self.mode_name(DATA)
            selection, x_range, auto, y_range, errors = self.plotting_canvas_widgets[old_plot_mode].get_quick_edit_info
            self.plotting_canvas_widgets[pane_to_match].set_quick_edit_info(selection, x_range, auto, y_range,
                                                                                  errors)

        if self._current_plot_mode==self.mode_name(RAW):
            # plot the raw data
            self.modes[RAW].handle_data_updated()

        self.presenter.hide(old_plot_mode)
        self.presenter.show(self._current_plot_mode)

    """ data pane """

    def create_data_pane(self):
        self.data_model = PlotDataPaneModel(self._context)
        # names need to match the model for the use outside of this file
        name = self.data_model.name
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.data_model)
        self._views[name] = BasePaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[DATA] = PlotDataPanePresenter(self._views[name], self.data_model,
                                                 self._context,self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[DATA])

    @property
    def data_index(self):
        return self.view.get_index(self.mode_name(DATA))

    @property
    def data_mode(self):
        return self.modes[DATA]

    """ Fit pane """
    def create_fit_pane(self):
        self.fit_model = PlotTimeFitPaneModel(self._context)
        name = self.fit_model.name
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.fit_model)
        self._views[name] = BasePaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[FIT] = PlotFitPanePresenter(self._views[name], self.fit_model,
                                               self._context,self._context.fitting_context,
                                               self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[FIT])

    @property
    def fit_index(self):
        return self.view.get_index(self.mode_name(FIT))

    @property
    def fit_mode(self):
        return self.modes[FIT]

    """ raw pane """
    def create_raw_pane(self):
        self.raw_model = RawPaneModel(self._context)
        name =self.raw_model.name
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.raw_model)
        self._views[name] = RawPaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[RAW] = RawPanePresenter(self._views[name], self.raw_model,
                                                 self._context,self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[RAW])

    @property
    def raw_index(self):
        if self.mode_name(RAW):
            return self.view.get_index(self.mode_name(RAW))
        else:
            # default to data if no model plot
            return self.view.get_index(self.mode_name(DATA))

    @property
    def raw_mode(self):
        return self.modes[RAW]

    """ model pane """
    def create_model_fit_pane(self):
        self.model_fit_model = PlotModelFitPaneModel(self._context)
        name = self.model_fit_model.name
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.model_fit_model)

        self._views[name] = BasePaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[MODEL] = PlotModelFitPanePresenter(self._views[name], self.model_fit_model, self._context,
                                                        self._context.model_fitting_context,
                                                        self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[MODEL])

    @property
    def model_fit_index(self):
        if self.mode_name(MODEL):
            return self.view.get_index(self.mode_name(MODEL))
        else:
            # default to fit if no model plot
            return self.view.get_index(self.mode_name(FIT))

    @property
    def model_fit_mode(self):
        return self.modes[MODEL]
