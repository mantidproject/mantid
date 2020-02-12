# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

PLOT_KWARGS = {"linestyle": "", "marker": "x", "markersize": "3"}


class FittingPlotPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

    def add_workspace_to_plot(self, ws):
        axes = self.view.get_axes()
        for ax in axes:
            self.model.add_workspace_to_plot(ws, ax, PLOT_KWARGS)
        self.view.update_figure()

    def remove_workspace_from_plot(self, ws):
        for ax in self.view.get_axes():
            self.model.remove_workspace_from_plot(ws, ax)
        self.view.update_figure()

    def clear_plot(self):
        for ax in self.view.get_axes():
            self.model.remove_all_workspaces_from_plot(ax)
        self.view.clear_figure()
