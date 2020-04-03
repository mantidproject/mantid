# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plotting_widget.plotting_widget_presenter1 import PlotWidgetPresenter1
from Muon.GUI.Common.plotting_widget.plotting_widget_view import PlotWidgetView
from Muon.GUI.Common.plotting_widget.plotting_widget_presenter import PlotWidgetPresenter
from Muon.GUI.Common.plotting_widget.plotting_widget_model import PlotWidgetModel
from Muon.GUI.Common.plotting_widget.plotting_widget_view1 import PlotWidgetView1


class PlottingWidget(object):
    def __init__(self, context=None, parent=None):
        self.view = PlotWidgetView(parent=parent)
        self.model = PlotWidgetModel()
        self.presenter = PlotWidgetPresenter(self.view,
                                             self.model,
                                             context)

        context.update_plots_notifier.add_subscriber(self.presenter.workspace_replaced_in_ads_observer)
        context.deleted_plots_notifier.add_subscriber(self.presenter.workspace_deleted_from_ads_observer)

    def close(self):
        self.view.close()


class PlottingWidget1(object):
    def __init__(self, context=None, parent=None):
        self.view = PlotWidgetView1(parent=parent)
        self.model = None
        self.presenter = PlotWidgetPresenter1(self.view,
                                              self.model,
                                              context)

        # context.update_plots_notifier.add_subscriber(self.presenter.workspace_replaced_in_ads_observer)
        # context.deleted_plots_notifier.add_subscriber(self.presenter.workspace_deleted_from_ads_observer)

    def close(self):
        self.view.close()
