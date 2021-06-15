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
from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_presenter import PlotTimeFitPanePresenter
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView


class MuonAnalysisPlotWidget(object):
    def __init__(self, context=None, get_active_fit_results=lambda: [], parent=None):
        # The plotting canvas widgets
        self.plotting_canvas_widget1 = PlottingCanvasWidget(parent, context=context)
        self.plotting_canvas_widget2 = PlottingCanvasWidget(parent, context=context)
        # The UI view
        self._view1 = BasePaneView(parent)
        self._view1.add_canvas_widget(self.plotting_canvas_widget1.widget)

        self._view2 = BasePaneView(parent)
        self._view2.add_canvas_widget(self.plotting_canvas_widget2.widget)
        # set up presenter
        self.view = MainPlotWidgetView(parent)
        self.model = PlotDataPaneModel(context)
        # generate the presenter

        self.data_mode = PlotDataPanePresenter(self._view1, self.model, context,self.plotting_canvas_widget1.presenter)
        self.fit_mode = PlotTimeFitPanePresenter(self._view2, self.model, context,self.plotting_canvas_widget2.presenter)

        self.presenter = MainPlotWidgetPresenter(self.view,
                                                   [self.data_mode, self.fit_mode])

        context.update_plots_notifier.add_subscriber(self.data_mode.workspace_replaced_in_ads_observer)
        context.deleted_plots_notifier.add_subscriber(self.data_mode.workspace_deleted_from_ads_observer)

    def close(self):
        self.view.close()
