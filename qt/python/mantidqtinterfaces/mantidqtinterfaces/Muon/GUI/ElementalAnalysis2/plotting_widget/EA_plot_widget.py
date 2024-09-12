# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_presenter import MainPlotWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import EAPlotDataPaneModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_presenter import (
    EAPlotDataPanePresenter,
)
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView


class EAPlotWidget(object):
    def __init__(self, context=None, get_active_fit_results=lambda: [], parent=None):
        self.data_model = EAPlotDataPaneModel(context)

        self.plotting_canvas_widgets = {
            self.data_model.name: PlottingCanvasWidget(
                parent, context=context.plot_panes_context[self.data_model.name], plot_model=self.data_model
            )
        }

        # The UI view
        self._view1 = BasePaneView(parent)
        self._view1.add_canvas_widget(self.plotting_canvas_widgets[self.data_model.name].widget)

        # set up presenter
        self.view = MainPlotWidgetView(parent)
        self.model = EAPlotDataPaneModel(context)
        # generate the presenter

        self.data_mode = EAPlotDataPanePresenter(
            self._view1, self.data_model, context, self.plotting_canvas_widgets[self.data_model.name].presenter
        )

        self.presenter = MainPlotWidgetPresenter(self.view, [self.data_mode])
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
    def workspace_deleted_from_ads_observers(self):
        return self.presenter.workspace_deleted_from_ads_observers

    @property
    def rebin_options_set_observers(self):
        return self.presenter.rebin_options_set_observers

    @property
    def data_index(self):
        return self.view.get_index(self.data_mode.name)

    def handle_plot_mode_changed_by_user(self):
        pass

    def close(self):
        self.view.close()
