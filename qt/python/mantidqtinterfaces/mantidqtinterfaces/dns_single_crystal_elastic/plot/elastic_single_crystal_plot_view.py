# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Widget to plot elastic single_crystal data
"""
from mantidqt.utils.qt import load_ui

from matplotlib.backends.backend_qt5agg import FigureCanvas
from matplotlib.backends.backend_qt5agg import \
    NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

from qtpy.QtCore import Signal
from qtpy.QtWidgets import QSizePolicy

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view \
    import DNSView
from mantidqtinterfaces.dns_sc_elastic.plot.dialogs.dialogs import (
    DNSdxdyDialog,
    DNSOmegaOffsetDialog)
from mantidqtinterfaces.dns_sc_elastic.plot.elastic_single_crystal_plot_menu \
    import DNSElasticSCPlotOptionsMenu, DNSElasticSCPlotViewMenu, \
    set_mdi_icons, set_up_colormap_selector
from mantidqtinterfaces.dns_sc_elastic.plot.elastic_single_crystal_plot_plot \
    import DNSScPlot
from mantidqtinterfaces.dns_sc_elastic.plot. \
    elastic_single_crystal_plot_datalist import DNSDatalist


class DNSElasticSCPlotView(DNSView):
    """
       DNS Widget to plot elastic single crystal data
    """
    NAME = 'Plotting'

    def __init__(self, parent):
        super().__init__(parent)
        _content = load_ui(__file__,
                           'elastic_single_crystal_plot.ui',
                           baseinstance=self)

        self._map = {
            'datalist': _content.lV_datalist,
            'down': _content.tB_down,
            'up': _content.tB_up,
            'grid': _content.tB_grid,
            'linestyle': _content.tB_linestyle,
            'crystal_axes': _content.tB_crystal_axes,
            'xrange': _content.lE_xrange,
            'yrange': _content.lE_yrange,
            'zrange': _content.lE_zrange,
            'colormap': _content.combB_colormap,
            'projections': _content.tB_projections,
            'log_scale': _content.tB_log,
            'invert_cb': _content.tB_invert_cb,
            'save_data': _content.tB_save_data,
            'fontsize': _content.sB_fontsize
        }
        # Change Toolbutton Icons to mdi icons
        set_mdi_icons(self._map)
        # Colormap Selector
        set_up_colormap_selector(self._map)

        # datalist
        self.datalist = DNSDatalist(self, self._map['datalist'])
        self._map['down'].clicked.connect(self.datalist.down)
        self._map['up'].clicked.connect(self.datalist.up)
        self.datalist.sig_datalist_changed.connect(self._something_changed)

        # Connecting Signals
        self._map['fontsize'].editingFinished.connect(
            self._change_fontsize)
        self._map['grid'].clicked.connect(self._change_grid)
        self._map['log_scale'].clicked.connect(self._change_log)
        self._map['linestyle'].clicked.connect(self._change_linestyle)
        self._map['projections'].clicked.connect(self._toggle_projections)
        # self._map['save_data'].clicked.connect(self.save_data)
        self._map['xrange'].returnPressed.connect(self._manual_lim_changed)
        self._map['yrange'].returnPressed.connect(self._manual_lim_changed)
        self._map['zrange'].returnPressed.connect(self._manual_lim_changed)
        self._map['colormap'].currentIndexChanged.connect(
            self._change_colormap)
        self._map['crystal_axes'].clicked.connect(
            self._change_crystal_axes)
        self._map['invert_cb'].clicked.connect(self._change_colormap)

        # Setting up custom menu for sc plot options and views
        self.views_menu = DNSElasticSCPlotViewMenu()
        options_menu = DNSElasticSCPlotOptionsMenu(self)
        self.menus = []
        self.menus.append(self.views_menu)
        self.menus.append(options_menu)
        self.views_menu.sig_replot.connect(self._plot)
        canvas = FigureCanvas(Figure())
        toolbar = NavigationToolbar(canvas, self)
        toolbar.actions()[0].triggered.connect(self._home_button_clicked)
        _content.plot_head_layout.insertWidget(5, toolbar)
        _content.plot_layout.insertWidget(2, canvas)
        canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.canvas = canvas
        self.sc_plot = DNSScPlot(self, self.canvas.figure, None)
        self.initial_values = None

    # Signals
    sig_plot = Signal()
    sig_update_omegaoffset = Signal(float)
    sig_update_dxdy = Signal(float, float)
    sig_calculate_projection = Signal(bool)
    sig_save_data = Signal()
    sig_change_grid = Signal()
    sig_change_crystal_axes = Signal(bool)
    sig_change_colormap = Signal()
    sig_change_log = Signal()
    sig_change_fontsize = Signal()
    sig_change_linestyle = Signal()
    sig_manual_lim_changed = Signal()
    sig_change_cb_range_on_zoom = Signal()
    sig_home_button_clicked = Signal()

    # emitting custom signals for presenter
    def _home_button_clicked(self):
        self.sig_home_button_clicked.emit()

    def change_cb_range_on_zoom(self, _dummy):
        # connected to ylim_changed of ax callback
        self.sig_change_cb_range_on_zoom.emit()

    def _manual_lim_changed(self):
        self.sig_manual_lim_changed.emit()

    def _change_linestyle(self):
        self.sig_change_linestyle.emit()

    def _toggle_projections(self, set_proj):
        self.sig_calculate_projection.emit(set_proj)

    def _change_fontsize(self):
        self.sig_change_fontsize.emit()

    def _change_log(self):
        self.sig_change_log.emit()

    def _change_colormap(self):
        self.sig_change_colormap.emit()

    def _change_crystal_axes(self, pressed):
        self.sig_change_crystal_axes.emit(pressed)

    def _plot(self):
        self.sig_plot.emit()

    def _change_grid(self):
        self.sig_change_grid.emit()

    def _save_data(self):
        self.sig_save_data.emit()

    def _something_changed(self):
        self.sig_plot.emit()

    # dialogs
    def change_dxdy(self):
        if self.initial_values:
            dxdy_dialog = DNSdxdyDialog(parent=self,
                                        dx=self.initial_values['dx'],
                                        dy=self.initial_values['dy'])
            dxdy_dialog.exec_()

    def change_omegaoffset(self):
        if self.initial_values:
            oo_dialog = DNSOmegaOffsetDialog(parent=self,
                                             omegaoffset=self.initial_values[
                                                 'oof'])
            oo_dialog.exec_()

    # gui options
    def create_subfigure(self, gridhelper=None):
        self.sc_plot = DNSScPlot(self, self.canvas.figure, gridhelper)

    def get_axis_type(self):
        return self.views_menu.get_value()

    def connect_resize(self):
        self.canvas.mpl_connect('resize_event', self.sc_plot.onresize)

    def set_initial_oof_dxdy(self, off, dx, dy):
        self.initial_values = {'oof': off,
                               'dx': dx,
                               'dy': dy}

    def draw(self):
        self.canvas.draw()
