# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

from Muon.GUI.Common.plotting_widget.external_plotting_model import PlotInformation
from mantid import AnalysisDataService

DEFAULT_X_LIMITS = [0, 15]


class PlottingCanvasModel(object):

    def __init__(self):
        self._user_axis_limits = None
        self.tiled_axes = {}

    def get_axis_limits(self, axes):
        if self._user_axis_limits is None:
            return self.get_default_limits(axes)

    def get_default_limits(self, axes):
        xmin = DEFAULT_X_LIMITS[0]
        xmax = DEFAULT_X_LIMITS[1]
        ymin, ymax = self._get_autoscale_y_limits(axes, xmin, xmax)
        return [xmin, xmax], [ymin, ymax]

    def _get_autoscale_y_limits(self, axes, xmin, xmax):
        new_bottom = 1e9
        new_top = -1e9
        for axis in axes:
            ylim = np.inf, -np.inf
            for line in axis.lines:
                x, y = line.get_data()
                start, stop = np.searchsorted(x, tuple([xmin, xmax]))
                y_within_range = y[max(start - 1, 0):(stop + 1)]
                ylim = min(ylim[0], np.nanmin(y_within_range)), max(ylim[1], np.nanmax(y_within_range))

            new_bottom_i = ylim[0] * 1.3 if ylim[0] < 0.0 else ylim[0] * 0.7
            new_top_i = ylim[1] * 1.3 if ylim[1] > 0.0 else ylim[1] * 0.7
            if new_bottom_i < new_bottom:
                new_bottom = new_bottom_i
            if new_top_i > new_top:
                new_top = new_top_i

        return new_bottom, new_top

    def create_plot_information(self, workspace_name, index, errors):
        A = self.tiled_axes
        try:
            workspace = AnalysisDataService.Instance().retrieve(workspace_name)
        except RuntimeError:
            return

        return PlotInformation(workspace=workspace, specNum=index + 1, axis=0, normalised=False,
                               errors=errors)
