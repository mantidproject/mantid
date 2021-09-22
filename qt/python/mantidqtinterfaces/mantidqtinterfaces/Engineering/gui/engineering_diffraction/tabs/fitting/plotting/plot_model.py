# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class FittingPlotModel(object):
    def __init__(self):
        self.plotted_workspaces = set()

    def get_plotted_workspaces(self):
        return self.plotted_workspaces

    def add_workspace_to_plot(self, ws, ax, plot_kwargs):
        ax.plot(ws, **plot_kwargs)
        self.plotted_workspaces.add(ws)

    def remove_workspace_from_plot(self, ws, ax):
        if ws in self.plotted_workspaces:
            self._remove_workspace_from_plot(ws, ax)
            self.plotted_workspaces.remove(ws)

    @staticmethod
    def _remove_workspace_from_plot(ws, ax):
        ax.remove_workspace_artists(ws)

    def remove_all_workspaces_from_plot(self, ax):
        ax.cla()
        self.plotted_workspaces.clear()
