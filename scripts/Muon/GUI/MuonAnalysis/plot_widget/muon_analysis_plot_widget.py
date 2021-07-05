# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from Muon.GUI.Common.plot_widget.main_plot_widget_presenter import MainPlotWidgetPresenter
from Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_presenter import PlotDataPanePresenter
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter
from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel


class MuonAnalysisPlotWidget(object):
    def __init__(self, context=None, get_active_fit_results=lambda: [], parent=None):

        self.data_model = PlotDataPaneModel(context)
        self.fit_model = PlotTimeFitPaneModel(context)

        # The plotting canvas widgets
        self.plotting_canvas_widgets = {}
        self.plotting_canvas_widgets[self.data_model.name] = PlottingCanvasWidget(parent, context=
                                                                                  context.plot_panes_context[self.data_model.name],
                                                                                  plot_model=self.data_model)
        self.plotting_canvas_widgets[self.fit_model.name] = PlottingCanvasWidget(parent, context=
                                                                                 context.plot_panes_context[self.fit_model.name],
                                                                                 plot_model=self.fit_model)
        # The UI view
        self._view1 = BasePaneView(parent)
        self._view1.add_canvas_widget(self.plotting_canvas_widgets[self.data_model.name].widget)

        self._view2 = BasePaneView(parent)
        self._view2.add_canvas_widget(self.plotting_canvas_widgets[self.fit_model.name].widget)
        # set up presenter
        self.view = MainPlotWidgetView(parent)
        self.model = PlotDataPaneModel(context)
        # generate the presenter

        self.data_mode = PlotDataPanePresenter(self._view1, self.data_model,
                                               context,self.plotting_canvas_widgets[self.data_model.name].presenter)
        self.fit_mode = PlotFitPanePresenter(self._view2, self.fit_model,
                                                 context,self.plotting_canvas_widgets[self.fit_model.name].presenter)

        self.presenter = MainPlotWidgetPresenter(self.view,
                                                   [self.data_mode, self.fit_mode])
        self._current_plot_mode = self.presenter.get_plot_mode
        self.presenter.set_plot_mode_changed_slot(self.handle_plot_mode_changed_by_user)

        for observer in self.presenter.workspace_replaced_in_ads_observers:
            context.update_plots_notifier.add_subscriber(observer)
        for observer in self.presenter.workspace_deleted_from_ads_observers:
            context.deleted_plots_notifier.add_subscriber(observer)

    def set_plot_view(self, plot_mode):
        self.view.set_plot_mode(plot_mode)

    @property
    def data_changed_observers(self):
        return self.presenter.data_changed_observers

    @property
    def rebin_options_set_observers(self):
        return self.presenter.rebin_options_set_observers

    @property
    def data_index(self):
        return self.view.get_index(self.data_mode.name)

    @property
    def fit_index(self):
        return self.view.get_index(self.fit_mode.name)

    def close(self):
        self.view.close()

    def handle_plot_mode_changed_by_user(self):
        old_plot_mode = self._current_plot_mode
        self._current_plot_mode = self.presenter.get_plot_mode
        selection, x_range, auto, y_range, errors = self.plotting_canvas_widgets[old_plot_mode].get_quick_edit_info
        self.plotting_canvas_widgets[self._current_plot_mode].set_quick_edit_info(selection, x_range, auto, y_range, errors)
        self.presenter.hide(old_plot_mode)
        self.presenter.show(self._current_plot_mode)
