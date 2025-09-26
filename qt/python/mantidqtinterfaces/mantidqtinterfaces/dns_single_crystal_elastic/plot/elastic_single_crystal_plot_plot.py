# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab of DNS reduction GUI.
"""

import numpy as np
import matplotlib
from matplotlib.ticker import AutoMinorLocator, NullLocator
from mpl_toolkits.axes_grid1 import make_axes_locatable
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
        self._cid = None

    def disconnect_ylim_change(self):
        if self._cid is not None:
            self._ax.callbacks.disconnect(self._cid)

    def connect_ylim_change(self):
        self._cid = self._ax.callbacks.connect("ylim_changed", self._parent.change_cb_range_on_zoom)

    def on_resize(self, _dummy=None):  # connected to canvas resize
        self._fig.tight_layout(pad=0.3)

    @staticmethod
    def set_fontsize(fontsize):
        matplotlib.rcParams.update({"font.size": fontsize})

    def redraw_colorbar(self):
        # called by home button clicked
        self._colorbar._draw_all()

    def create_colorbar(self):
        self._colorbar = self._fig.colorbar(self._plot, ax=self._ax, extend="max", pad=0.01)

    def set_norm(self, norm):
        self._plot.set_norm(norm)

    def set_linecolor(self, lines):
        lines = ["face", "white", "black"][lines]
        if self._plot is not None:
            self._plot.set_edgecolor(lines)

    def set_pointsize(self, lines):
        if self._plot is not None:
            a = np.ones(len(self._plot.get_sizes()))
            size = a * [10, 50, 100, 500, 1000][lines]
            self._plot.set_sizes(size)

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

    def set_format_coord(self, format_coord):
        self._ax.format_coord = format_coord

    def set_xlim(self, xlim):
        if xlim[0] is None:
            self._ax.autoscale(axis="x")
        else:
            self._ax.set_xlim(left=xlim[0], right=xlim[1], auto=True)

    def set_ylim(self, ylim):
        if ylim[0] is None:
            self._ax.autoscale(axis="y")
        else:
            self._ax.set_ylim(bottom=ylim[0], top=ylim[1], auto=True)

    def set_grid(self, major=False, minor=False):
        if minor:
            self._ax.xaxis.set_minor_locator(AutoMinorLocator(5))
        else:
            self._ax.xaxis.set_minor_locator(NullLocator())
        if major:
            self._ax.grid(True, which="both", zorder=1000, linestyle="--")
        else:
            self._ax.grid(0)

    def set_aspect_ratio(self, aspect_ratio):
        self._ax.set_aspect(aspect_ratio, anchor="NW")

    # projections
    def set_projections(self, x_proj, y_proj, xlim, ylim):
        self.remove_projections()
        divider = make_axes_locatable(self._ax)
        self._ax_hist[0] = divider.append_axes("top", 1.2, pad=0.1, sharex=self._ax)
        self._ax_hist[1] = divider.append_axes("right", 1.2, pad=0.1, sharey=self._ax)
        self._ax.set_xlim(xlim[0], xlim[1])
        self._ax.set_ylim(ylim[0], ylim[1])
        self._ax.set_xmargin(0)
        self._ax.set_ymargin(0)
        self._ax_hist[0].set_xmargin(0)
        self._ax_hist[1].set_ymargin(0)
        self._ax_hist[0].axis["bottom"].major_ticklabels.set_visible(False)
        self._ax_hist[1].axis["left"].major_ticklabels.set_visible(False)
        self._ax_hist[0].plot(x_proj[0], x_proj[1])
        self._ax_hist[1].plot(y_proj[1], y_proj[0])

    def remove_projections(self):
        if hasattr(self, "_ax_hist") and self._ax_hist[0] is not None:
            try:
                self._ax_hist[0].remove()
                self._ax_hist[1].remove()
            except KeyError:
                pass

    # getting stuff
    def get_active_limits(self):
        xlim = self._ax.get_xlim()
        ylim = self._ax.get_ylim()
        return xlim, ylim

    # plotting
    def clear_plot(self):
        if self._ax:
            self._ax.figure.clear()

    def plot_triangulation(self, triang, z, cmap, edge_colors, shading):
        self._ax.set_visible(True)
        self._plot = self._ax.tripcolor(triang, z, cmap=cmap, edgecolors=edge_colors, shading=shading)

    def plot_quadmesh(self, x, y, z, cmap, edge_colors, shading):
        # pylint: disable=too-many-arguments
        self._ax.set_visible(True)
        self._plot = self._ax.pcolormesh(x, y, z, cmap=cmap, edgecolors=edge_colors, shading=shading)
        # set 5% padding to pcolormesh, similar to default values in tripcolor and scatter plots
        x0, x1 = self._ax.get_xlim()
        y0, y1 = self._ax.get_ylim()
        dx = (x1 - x0) * 0.05
        dy = (y1 - y0) * 0.05
        self._ax.set_xlim(x0 - dx, x1 + dx)
        self._ax.set_ylim(y0 - dy, y1 + dy)

    def plot_scatter(self, x, y, z, cmap):
        self._ax.set_visible(True)
        self._plot = self._ax.scatter(x, y, c=z, s=100, cmap=cmap, zorder=100)
