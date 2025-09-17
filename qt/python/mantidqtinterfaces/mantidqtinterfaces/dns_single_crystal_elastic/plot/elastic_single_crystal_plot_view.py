# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab view of DNS reduction GUI.
"""

from mantidqt.utils.qt import load_ui
from matplotlib.backends.backend_qt5agg import FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QSizePolicy
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView
from mantidqtinterfaces.dns_single_crystal_elastic.plot.dialogs.dialogs import DNSdxdyDialog, DNSOmegaOffsetDialog
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_menu import (
    DNSElasticSCPlotOptionsMenu,
    DNSElasticSCPlotViewMenu,
    set_mdi_icons,
    set_up_colormap_selector,
)
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_plot import DNSScPlot
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_datalist import DNSDatalist


class DNSElasticSCPlotView(DNSView):
    """
    DNS Widget to plot elastic single crystal data
    """

    NAME = "Plotting"

    def __init__(self, parent):
        super().__init__(parent)
        _content = load_ui(__file__, "elastic_single_crystal_plot.ui", baseinstance=self)

        self._map = {
            "datalist": _content.lV_datalist,
            "down": _content.tB_down,
            "up": _content.tB_up,
            "grid": _content.tB_grid,
            "linestyle": _content.tB_linestyle,
            "crystal_axes": _content.tB_crystal_axes,
            "x_min": _content.sB_x_min,
            "x_max": _content.sB_x_max,
            "y_min": _content.sB_y_min,
            "y_max": _content.sB_y_max,
            "z_min": _content.sB_z_min,
            "z_max": _content.sB_z_max,
            "colormap": _content.combB_colormap,
            "projections": _content.tB_projections,
            "log_scale": _content.tB_log,
            "invert_cb": _content.tB_invert_cb,
            "save_data": _content.tB_save_data,
            "fontsize": _content.sB_fontsize,
        }
        # change tool button icons to mdi icons
        set_mdi_icons(self._map)
        # colormap selector
        set_up_colormap_selector(self._map)

        # datalist
        self.datalist = DNSDatalist(self, self._map["datalist"])
        self._map["down"].clicked.connect(self.datalist.down)
        self._map["up"].clicked.connect(self.datalist.up)
        self.datalist.sig_datalist_changed.connect(self._plot)

        # connecting signals
        self._attach_signal_slots()

        # setting up custom menu for single crystal plot options and views
        self.views_menu = DNSElasticSCPlotViewMenu()
        self.options_menu = DNSElasticSCPlotOptionsMenu(self)
        self.set_plot_view_menu_visibility(False)
        self.set_plot_options_menu_visibility(False)
        self.menus = []
        self.menus.append(self.views_menu)
        self.menus.append(self.options_menu)
        self.views_menu.sig_replot.connect(self._plot)
        self.canvas = FigureCanvas(Figure())
        self.canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.toolbar = NavigationToolbar(self.canvas, self)
        self.toolbar.actions()[0].triggered.connect(self._home_button_clicked)
        _content.plot_head_layout.insertWidget(5, self.toolbar)
        _content.plot_layout.insertWidget(2, self.canvas)
        self.canvas.mpl_connect("button_release_event", self._zooming_event)
        self.single_crystal_plot = DNSScPlot(self, self.canvas.figure, None)
        self.initial_values = None

    # Signals
    sig_plot = Signal()
    sig_restore_default_omega_offset = Signal()
    sig_restore_default_dxdy = Signal()
    sig_update_omega_offset = Signal(float)
    sig_update_dxdy = Signal(float, float)
    sig_calculate_projection = Signal(bool)
    sig_save_data = Signal()
    sig_change_grid = Signal()
    sig_change_crystal_axes = Signal(bool)
    sig_change_colormap = Signal()
    sig_change_log = Signal()
    sig_change_font_size = Signal()
    sig_change_linestyle = Signal()
    sig_manual_lim_changed = Signal()
    sig_change_cb_range_on_zoom = Signal()
    sig_home_button_clicked = Signal()
    sig_plot_zoom_updated = Signal()

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

    def _change_font_size(self):
        self.sig_change_font_size.emit()

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

    def set_plot_view_menu_visibility(self, visible):
        self.views_menu.menuAction().setVisible(visible)

    def set_plot_options_menu_visibility(self, visible):
        self.options_menu.menuAction().setVisible(visible)

    # dialogs
    def change_dxdy(self):
        if self.initial_values:
            dxdy_dialog = DNSdxdyDialog(parent=self, dx=self.initial_values["dx"], dy=self.initial_values["dy"])
            dxdy_dialog.exec_()

    def change_omega_offset(self):
        if self.initial_values:
            omega_offset_dialog = DNSOmegaOffsetDialog(parent=self, omega_offset=self.initial_values["omega_offset"])
            omega_offset_dialog.exec_()

    # gui options
    def _zooming_event(self, event):
        # event.button = 1 for zooming in, = 3 for zooming out
        if (event.button == 1 or event.button == 3) and self.toolbar.mode == "zoom rect":
            self.sig_plot_zoom_updated.emit()

    def create_subfigure(self, grid_helper=None):
        self.single_crystal_plot = DNSScPlot(self, self.canvas.figure, grid_helper)

    def get_axis_type(self):
        return self.views_menu.get_value()

    def connect_resize(self):
        self.canvas.mpl_connect("resize_event", self.single_crystal_plot.on_resize)

    def set_initial_omega_offset_dx_dy(self, off, dx, dy):
        self.initial_values = {"omega_offset": off, "dx": dx, "dy": dy}

    def draw(self):
        self.canvas.draw()

    def _attach_signal_slots(self):
        self._map["fontsize"].valueChanged.connect(self._change_font_size)
        self._map["grid"].clicked.connect(self._change_grid)
        self._map["log_scale"].clicked.connect(self._change_log)
        self._map["linestyle"].clicked.connect(self._change_linestyle)
        self._map["projections"].clicked.connect(self._toggle_projections)
        self._map["save_data"].clicked.connect(self._save_data)
        self._map["x_min"].valueChanged.connect(self._manual_lim_changed)
        self._map["x_max"].valueChanged.connect(self._manual_lim_changed)
        self._map["y_min"].valueChanged.connect(self._manual_lim_changed)
        self._map["y_max"].valueChanged.connect(self._manual_lim_changed)
        self._map["z_min"].valueChanged.connect(self._manual_lim_changed)
        self._map["z_max"].valueChanged.connect(self._manual_lim_changed)
        self._map["colormap"].currentIndexChanged.connect(self._change_colormap)
        self._map["crystal_axes"].clicked.connect(self._change_crystal_axes)
        self._map["invert_cb"].clicked.connect(self._change_colormap)
