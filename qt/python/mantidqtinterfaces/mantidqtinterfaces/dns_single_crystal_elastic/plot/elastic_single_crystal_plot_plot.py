# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab of DNS reduction GUI.
"""

import matplotlib
from mpl_toolkits.axisartist import Subplot


class DNSScPlot:
    def __init__(self, parent, figure, grid_helper):
        super().__init__()
        self._fig = figure
        self._fig.clf()
        ax1 = Subplot(self._fig, 1, 1, 1, grid_helper=grid_helper)
        self._ax = self._fig.add_subplot(ax1)
        self._ax.set_visible(False)
        self._parent = parent
        self._fig = figure
        self._plot = None
        self._colorbar = None
        self._ax_hist = [None, None]

    def on_resize(self, _dummy=None):  # connected to canvas resize
        self._fig.tight_layout(pad=0.3)

    @staticmethod
    def set_fontsize(fontsize):
        matplotlib.rcParams.update({"font.size": fontsize})

    def create_colorbar(self):
        self._colorbar = self._fig.colorbar(self._plot, ax=self._ax, extend="max", pad=0.01)

    def set_norm(self, norm):
        self._plot.set_norm(norm)

    def set_shading(self, shading):
        if self._plot is not None:
            self._plot.set_shading(shading)

    def set_zlim(self, zlim):
        if self._plot is not None:
            self._plot.set_clim(zlim[0], zlim[1])
            self._colorbar.mappable.set_clim(vmin=zlim[0], vmax=zlim[1])

    def set_cmap(self, cmap):
        if self._plot is not None:
            self._plot.set_cmap(cmap)

    def set_axis_labels(self, x_label, y_label):
        self._ax.set_xlabel(x_label)
        self._ax.set_ylabel(y_label)

    def plot_triangulation(self, triang, z, cmap, edge_colors, shading):
        self._ax.set_visible(True)
        self._plot = self._ax.tripcolor(triang, z, cmap=cmap, edgecolors=edge_colors, shading=shading)
