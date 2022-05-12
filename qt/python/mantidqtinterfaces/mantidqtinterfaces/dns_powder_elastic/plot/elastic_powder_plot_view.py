# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Widget to plot elastic powder data
"""

from threading import Timer

from mantidqt import icons
from mantidqt.utils.qt import load_ui
from matplotlib.backends.backend_qt5agg import FigureCanvas
from matplotlib.backends.backend_qt5agg import \
    NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.ticker import AutoMinorLocator, NullLocator
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QSizePolicy

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_plot_list \
    import DNSPlotListModel
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView

LINESTYLES = {
    0: '-',
    1: '.',
    2: '.-'}


class DNSElasticPowderPlotView(DNSView):
    """
       DNS Widget to plot elastic powder data
    """
    NAME = 'Plotting'

    def __init__(self, parent):
        super().__init__(parent)
        content = load_ui(__file__,
                          'elastic_powder_plot.ui',
                          baseinstance=self)
        layout = content.plot_layout
        self.static_canvas = FigureCanvas(Figure(figsize=(5, 3), dpi=200))
        self.static_canvas.setSizePolicy(QSizePolicy.Expanding,
                                         QSizePolicy.Expanding)
        toolbar = NavigationToolbar(self.static_canvas, self)
        content.plot_head_layout.addWidget(toolbar)
        content.plot_head_layout.addStretch()

        layout.addWidget(self.static_canvas)
        self._map = {
            'datalist': content.lV_datalist,
            'down': content.tB_down,
            'up': content.tB_up,
            'raw': content.tB_raw,
            'separated': content.tB_separated,
            'deselect': content.tB_deselect,
            'grid': content.tB_grid,
            'log_scale': content.cB_log_scale,
            'linestyle': content.tB_linestyle,
            'errorbar': content.tB_errorbar,
            'xaxis_scale': content.combB_xaxis_scale,
        }

        self._map['down'].setIcon(icons.get_icon("mdi.arrow-down"))
        self._map['up'].setIcon(icons.get_icon("mdi.arrow-up"))
        self._map['deselect'].setIcon(icons.get_icon("mdi.close"))
        self._map['grid'].setIcon(icons.get_icon("mdi.grid"))
        self._map['linestyle'].setIcon(icons.get_icon("mdi.ray-vertex"))
        self._map['errorbar'].setIcon(icons.get_icon("mdi.format-size"))

        self.datalist = self._map['datalist']
        self.ax = None
        self.static_canvas.figure.tight_layout()
        self.model = DNSPlotListModel(self.datalist)
        self.model.itemChanged.connect(self._something_changed)
        self._map['down'].clicked.connect(self.model.down)
        self._map['up'].clicked.connect(self.model.up)
        self._map['deselect'].clicked.connect(self._uncheck_items)
        self._map['separated'].clicked.connect(self.check_seperated)
        self._map['grid'].clicked.connect(self._gridstate_change)
        self._map['log_scale'].stateChanged.connect(self._log_change)
        self._map['raw'].clicked.connect(self.check_raw)
        self._map['linestyle'].clicked.connect(self._linestyle_change)
        self._map['errorbar'].clicked.connect(self._errorbar_change)
        self._map['xaxis_scale'].currentIndexChanged.connect(
            self._something_changed)

    sig_plot = Signal()
    sig_gridstate_change = Signal(bool)  # bool = if to draw
    sig_errorbar_change = Signal()
    sig_linestyle_change = Signal()
    sig_log_change = Signal(bool)  # bool = if logscale

    def _log_change(self):
        log = self._map['log_scale'].checkState()
        self.sig_log_change.emit(log)

    def _linestyle_change(self):
        self.sig_linestyle_change.emit()

    def _errorbar_change(self):
        self.sig_errorbar_change.emit()

    def _gridstate_change(self, _dummy, draw=True):
        self.sig_gridstate_change.emit(draw)

    def check_first(self):
        self.model.check_first()

    def _uncheck_items(self):
        self.model.itemChanged.disconnect()
        self.model.uncheck_items()
        self.model.itemChanged.connect(self._something_changed)
        self._something_changed()

    def check_seperated(self):
        self.model.itemChanged.disconnect()
        self.model.check_seperated()
        self.model.itemChanged.connect(self._something_changed)
        self._something_changed()

    def check_raw(self):
        self.model.itemChanged.disconnect()
        self.model.check_raw()
        self.model.itemChanged.connect(self._something_changed)
        self._something_changed()

    def _something_changed(self):
        self.sig_plot.emit()

    def get_datalist(self):
        return self.model.get_names()

    def set_datalist(self, datalist):
        self.model.itemChanged.disconnect()
        self.model.set_items(datalist)
        self.datalist.setModel(self.model)
        self.model.itemChanged.connect(self._something_changed)

    def clear_plot(self):
        if self.ax:
            self.ax.figure.clear()

    def get_xaxis(self):
        return self._map['xaxis_scale'].currentText()

    def _plot(self):
        self.sig_plot.emit()

    def set_yscale(self, scale):
        self.ax.set_yscale(scale)
        self.draw()

    def set_no_grid(self):
        if self.ax is None:
            return
        self.ax.xaxis.set_minor_locator(NullLocator())
        self.ax.grid(0)

    def set_major_minor_grid(self):
        if self.ax is None:
            return
        self.ax.xaxis.set_minor_locator(AutoMinorLocator(5))
        self.ax.grid(1, which='both', zorder=-1000, linestyle='--')

    def set_major_grid(self):
        if self.ax is None:
            return
        self.ax.xaxis.set_minor_locator(NullLocator())
        self.ax.grid(1, which='both', zorder=-1000, linestyle='--')

    def draw(self):
        self.ax.figure.canvas.draw()

    def get_check_plots(self):
        return self.model.get_checked_item_names()

    def single_error_plot(self, x, y, yerr, label, capsize, linestyle):
        # pylint: disable=too-many-arguments
        self.ax.errorbar(x,
                         y,
                         yerr=yerr,
                         fmt=LINESTYLES[linestyle],
                         label=label,
                         capsize=capsize)

    def single_plot(self, x, y, label, linestyle):
        self.ax.plot(x, y, LINESTYLES[linestyle], label=label)

    def create_plot(self, norm):
        if self.ax:
            self.ax.figure.clear()
        self.ax = self.static_canvas.figure.subplots()
        self.ax.set_xlabel('2 theta (degree)')
        self.ax.set_ylabel(f'Intensity ({norm})')

    def finish_plot(self, xaxis):
        if self.model.get_checked_item_names():
            self.ax.legend()
        self.static_canvas.figure.tight_layout()
        self.ax.set_xlabel(xaxis)
        self._gridstate_change(0, draw=False)
        self._log_change()

    def start_timer(self):
        t = Timer(0.01, self._on_timer)  # wait until plot is draw
        t.start()

    def _on_timer(self):
        self.static_canvas.figure.tight_layout()
        self.draw()
