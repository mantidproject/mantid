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
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_presenter import PlotFreqFitPanePresenter
from Muon.GUI.Common.plot_widget.raw_pane.raw_pane_presenter import RawPanePresenter
from Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model import RawPaneModel
from Muon.GUI.Common.plot_widget.raw_pane.raw_pane_view import RawPaneView
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_presenter import DuelPlotMaxentPanePresenter
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_model import DuelPlotMaxentPaneModel
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_view import DuelPlotMaxentPaneView
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import DuelQuickEditWidget


class FrequencyAnalysisPlotWidget(object):
    def __init__(self, context=None, get_active_fit_results=lambda: [], parent=None):
        self.data_model = PlotDataPaneModel(context)
        self.fit_model = PlotFreqFitPaneModel(context)
        self.raw_model = RawPaneModel(context)
        self.plot_maxent_model = DuelPlotMaxentPaneModel(context, self.data_model, RawPaneModel(context))
        models = [self.data_model, self.fit_model, self.raw_model, self.plot_maxent_model]

        self.view = MainPlotWidgetView(parent)
        self.model = PlotDataPaneModel(context)
        # The plotting canvas widgets
        self.plotting_canvas_widgets = {}
        # The UI view
        self._views = {}

        for model in models:

            if model == self.plot_maxent_model:
                duel_quick_edit = DuelQuickEditWidget(context.plot_panes_context[model.name], parent)
                self.plotting_canvas_widgets[model.name] = PlottingCanvasWidget(parent, context=
                                                                                 context.plot_panes_context[model.name],
                                                                                 plot_model=model, figure_options=duel_quick_edit)
                self._views[model.name] = DuelPlotMaxentPaneView(parent)

            elif model == self.raw_model:
                self.plotting_canvas_widgets[model.name] = PlottingCanvasWidget(parent, context=
                                                                                 context.plot_panes_context[model.name],
                                                                                 plot_model=model)
                self._views[model.name] = RawPaneView(parent)

            else:
                self.plotting_canvas_widgets[model.name] = PlottingCanvasWidget(parent, context=
                                                                                 context.plot_panes_context[model.name],
                                                                                 plot_model=model)
                self._views[model.name] = BasePaneView(parent)
            self._views[model.name].add_canvas_widget(self.plotting_canvas_widgets[model.name].widget)

        self.plotting_canvas_widgets[self.raw_model.name].disable_plot_selection()
        # set up presenter

        # generate the presenters
        name = self.data_model.name
        self.data_mode = PlotDataPanePresenter(self._views[name], self.data_model,
                                               context,self.plotting_canvas_widgets[name].presenter)

        name = self.fit_model.name
        self.fit_mode = PlotFreqFitPanePresenter(self._views[name], self.fit_model,
                                                 context,context.fitting_context,
                                                 self.plotting_canvas_widgets[name].presenter)

        name = self.raw_model.name
        self.raw_mode = RawPanePresenter(self._views[name], self.raw_model,
                                                 context,self.plotting_canvas_widgets[name].presenter)

        name = self.plot_maxent_model.name
        self.maxent_mode = DuelPlotMaxentPanePresenter(self._views[name], self.plot_maxent_model,
                                                       context,self.plotting_canvas_widgets[name].presenter)

        self.presenter = MainPlotWidgetPresenter(self.view,
                                                   [self.data_mode, self.raw_mode, self.fit_mode, self.maxent_mode])

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
        return [self.data_mode.data_changed_observer]

    @property
    def rebin_options_set_observers(self):
        return [self.data_mode.rebin_options_set_observer]

    @property
    def data_index(self):
        return self.view.get_index(self.data_mode.name)

    @property
    def frequency_index(self):
        return self.view.get_index(self.fit_mode.name)

    @property
    def raw_index(self):
        self.view.get_index(self.raw_mode.name)

    def close(self):
        self.view.close()

    def handle_plot_mode_changed_by_user(self):
        old_plot_mode = self._current_plot_mode
        self._current_plot_mode = self.presenter.get_plot_mode
        self.presenter.hide(old_plot_mode)
        self.presenter.show(self._current_plot_mode)
