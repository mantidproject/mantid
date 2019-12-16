# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path Configuration Widget = View - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)
import os
from qtpy.QtWidgets import QSizePolicy
from qtpy.QtGui import QIcon

from matplotlib.backends.backend_qt5agg import (FigureCanvas,
                                                NavigationToolbar2QT as
                                                NavigationToolbar)
from matplotlib.figure import Figure
import matplotlib.colors as colors
from matplotlib.ticker import LogFormatter, ScalarFormatter

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui

from DNSReduction.plot.colormaps.matplotcolors import colormaps
from DNSReduction.data_structures.dns_view import DNSView


class DNSTofPowderPlot_view(DNSView):
    """
        Widget for basic plotting of DNS powder TOF data
    """
    ## Widget name
    name = "Paths"

    def __init__(self, parent):
        super(DNSTofPowderPlot_view, self).__init__(parent)
        self._content = load_ui(__file__,
                                'tof_powder_plot.ui',
                                baseinstance=self)
        self.name = 'Plotting'
        self.xml_filepath = None
        self.main_view = parent
        self.has_tab = True
        self.layout = self._content.plot_layout
        self.static_canvas = FigureCanvas(Figure(figsize=(5, 3), dpi=200))
        self.static_canvas.setSizePolicy(QSizePolicy.Expanding,
                                         QSizePolicy.Expanding)
        self.toolbar = NavigationToolbar(self.static_canvas, self)
        self.plot_head_layout.addWidget(self.toolbar)
        self.layout.addWidget(self.static_canvas)
        self._mapping = {
            'sl_min': self._content.sl_min,
            'sl_max': self._content.sl_max,
            'log_scale': self._content.cB_log_scale,
            'colormap': self._content.combB_colormap,
            'reverse_colormap': self._content.cB_reverse_colormap,
        }
        self._mapping['sl_min'].valueChanged.connect(self.update_min)
        self._mapping['sl_max'].valueChanged.connect(self.update_max)
        self._mapping['log_scale'].stateChanged.connect(self.set_log)
        self.hasplot = False
        self.ax = None
        self.cb = None
        self.plotmin = None
        self.cl = None
        self.workspace = None
        self.plotmax = None
        self.minimum = None
        self.maximum = None
        self.hasplot = False
        plotdir = os.path.dirname(__file__)
        for m in colormaps:
            self._mapping['colormap'].addItem(
                QIcon("{}/colormaps/{}.png".format(plotdir, m)), m)

        self._mapping['colormap'].setCurrentIndex(colormaps.index('viridis'))
        self._mapping['colormap'].currentIndexChanged.connect(
            self.set_colormap)
        self._mapping['reverse_colormap'].stateChanged.connect(
            self.set_colormap)
        self.static_canvas.figure.tight_layout()

    def clear_plot(self):
        if self.ax:
            self.ax.figure.clear()
        self.hasplot = False

    def set_log(self):

        own_dict = self.get_state()
        if own_dict['log_scale']:
            norm = colors.SymLogNorm(linthresh=0.1,
                                     linscale=1,
                                     vmin=self.minimum,
                                     vmax=self.maximum)
            myformatter = LogFormatter(10, labelOnlyBase=True)

        else:
            norm = colors.Normalize(vmin=self.minimum, vmax=self.maximum)
            myformatter = ScalarFormatter()
        if self.cl is not None:
            self.cb.set_norm(norm)
            self.cl.set_norm(norm)
            self.cb.formatter = myformatter
            self.cb.update_ticks()
            self.ax.figure.canvas.draw()

    def set_colormap(self):
        own_dict = self.get_state()
        colormap_name = own_dict['colormap']
        if own_dict['reverse_colormap']:
            colormap_name += '_r'
        if self.hasplot:
            self.cl.set_cmap(colormap_name)
            self.ax.figure.canvas.draw()

    def set_plot(self, workspace):
        if self.ax:
            self.ax.figure.clear()
        self.ax = self.static_canvas.figure.subplots(
            subplot_kw={'projection': 'mantid'})
        self.workspace = workspace
        self.ax.set_title('Inelastic Powder TOF')
        norm = colors.Normalize(vmin=self.minimum, vmax=self.maximum)
        self.cl = self.ax.pcolormesh(workspace, norm=norm)
        self.cb = self.static_canvas.figure.colorbar(self.cl)
        self.cb.set_label('Intensity normed to monitor')
        self.set_colormap()
        self.plotmin, self.plotmax = self.cl.get_clim()
        self.minimum = self.plotmin
        self.maximum = self.plotmax
        self.hasplot = True
        self.cl.set_clim(vmin=self.minimum, vmax=self.maximum)
        self.static_canvas.figure.tight_layout()
        self.set_log()

    def update_min(self, slidervalue):
        if self.cl is None:
            return
        self.minimum = (self.plotmax
                        - self.plotmin) * slidervalue / 1000.0 + self.plotmin
        self.cl.set_clim(vmin=self.minimum, vmax=self.maximum)
        self.ax.figure.canvas.draw()
        self.static_canvas.figure.tight_layout()

    def update_max(self, slidervalue):
        if self.cl is None:
            return
        self.maximum = (self.plotmax
                        - self.plotmin) * slidervalue / 1000.0 + self.plotmin
        self.cl.set_clim(vmin=self.minimum, vmax=self.maximum)
        self.ax.figure.canvas.draw()
        self.static_canvas.figure.tight_layout()
