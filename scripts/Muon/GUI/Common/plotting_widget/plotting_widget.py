# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,k
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.plotting_widget.plotting_widget_view import PlotWidgetView
from Muon.GUI.Common.plotting_widget.plotting_widget_presenter import PlotWidgetPresenter
from Muon.GUI.Common.plotting_widget.plotting_widget_model import PlotWidgetModel


class PlottingWidget(object):
    def __init__(self, context=None):
        # initialise the view, presenter and model.
        # view
        self.view = PlotWidgetView(parent=None)
        # model
        self.model = PlotWidgetModel(self.view.get_fig())
        # presenter
        self.presenter = PlotWidgetPresenter(self.view,
                                             self.model,
                                             context)

        context.update_view_from_model_notifier.add_subscriber(self.presenter.workspace_deleted_from_ads_observer)
        context.update_plots_notifier.add_subscriber(self.presenter.workspace_replaced_in_ads_observer)

    def close(self):
        self.view.close()
