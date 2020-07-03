# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from Muon.GUI.Common.plot_widget.plot_widget_presenter import PlotWidgetPresenterCommon
from Muon.GUI.Common.plot_widget.plot_widget_view import PlotWidgetView
from Muon.GUI.Common.plot_widget.plot_widget_model import PlotWidgetModel as PlotWidgetModel


class PlotWidget(object):
    def __init__(self, context=None, get_selected_fit_workspaces=None, parent=None):
        # The plotting canvas widget
        self.plotting_canvas_widget = PlottingCanvasWidget(parent, context=context)
        # The UI view
        self.view = PlotWidgetView(parent)
        self.view.add_canvas_widget(self.plotting_canvas_widget.widget)
        self.model = PlotWidgetModel(context)
        # generate the presenter
        self.presenter = PlotWidgetPresenterCommon(self.view,
                                                   self.model,
                                                   context,
                                                   self.plotting_canvas_widget.presenter,
                                                   get_selected_fit_workspaces)

        context.update_plots_notifier.add_subscriber(self.presenter.workspace_replaced_in_ads_observer)
        context.deleted_plots_notifier.add_subscriber(self.presenter.workspace_deleted_from_ads_observer)

    @property
    def plot_canvas_widget(self):
        return self.plotting_canvas_widget

    def close(self):
        self.view.close()
