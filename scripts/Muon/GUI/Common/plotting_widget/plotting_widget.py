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
        self.view = PlotWidgetView(parent=None)
        self.model = PlotWidgetModel()
        self.presenter = PlotWidgetPresenter(self.view,
                                             self.model,
                                             context)

        context.update_plots_notifier.add_subscriber(self.presenter.workspace_replaced_in_ads_observer)
        context.deleted_plots_notifier.add_subscriber(self.presenter.workspace_deleted_from_ads_observer)

    def close(self):
        self.view.close()
