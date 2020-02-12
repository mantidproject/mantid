# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_model import FittingPlotModel
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_view import FittingPlotView
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_presenter import FittingPlotPresenter

from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObserver


class FittingPlotWidget(object):
    def __init__(self, parent, view=None):
        if view is None:
            self.view = FittingPlotView(parent)
        else:
            self.view = view
        self.model = FittingPlotModel()
        self.presenter = FittingPlotPresenter(self.model, self.view)

        self.workspace_added_observer = GenericObserverWithArgPassing(self.presenter.add_workspace_to_plot)
        self.workspace_removed_observer = GenericObserverWithArgPassing(self.presenter.remove_workspace_from_plot)
        self.all_workspaces_removed_observer = GenericObserver(self.presenter.clear_plot)
