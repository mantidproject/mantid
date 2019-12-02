# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Widget to plot elastic powder data
"""
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict

from qtpy.QtWidgets import QSizePolicy
from qtpy.QtCore import Signal

from matplotlib.backends.backend_qt5agg import (FigureCanvas,
                                                NavigationToolbar2QT as
                                                NavigationToolbar)
from matplotlib.figure import Figure
from matplotlib.ticker import AutoMinorLocator, NullLocator
from mantidqt import icons
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui
from DNSReduction.data_structures.dns_view import DNSView
from DNSReduction.data_structures.dns_plot_list import DNSPlotListModel


class DNSElasticPowderPlot_view(DNSView):
    """
       DNS Widget to plot elastic powder data
    """
    ## Widget name
    name = "Paths"

    def __init__(self, parent):
        super(DNSElasticPowderPlot_view, self).__init__(parent)
        self._content = load_ui(__file__,
                                'elastic_powder_plot.ui',
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
        self.plot_head_layout.addStretch()

        self.layout.addWidget(self.static_canvas)
        self._mapping = {
            'datalist': self._content.lV_datalist,
            'down': self._content.tB_down,
            'up': self._content.tB_up,
            'raw': self._content.tB_raw,
            'separated': self._content.tB_separated,
            'deselect': self._content.tB_deselect,
            'grid': self._content.tB_grid,
            'log_scale': self._content.cB_log_scale,
            'linestyle': self._content.tB_linestyle,
            'errorbar': self._content.tB_errorbar,
            'xaxis_scale': self._content.combB_xaxis_scale,
        }

        self._mapping['down'].setIcon(icons.get_icon("mdi.arrow-down"))
        self._mapping['up'].setIcon(icons.get_icon("mdi.arrow-up"))
        self._mapping['deselect'].setIcon(icons.get_icon("mdi.close"))
        self._mapping['grid'].setIcon(icons.get_icon("mdi.grid"))
        self._mapping['linestyle'].setIcon(icons.get_icon("mdi.ray-vertex"))
        self._mapping['errorbar'].setIcon(icons.get_icon("mdi.format-size"))

        self.datalist = self._mapping['datalist']
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
        self.static_canvas.figure.tight_layout()
        self.model = DNSPlotListModel(self.datalist)
        self.model.itemChanged.connect(self.something_changed)
        self._mapping['down'].clicked.connect(self.model.down)
        self._mapping['up'].clicked.connect(self.model.up)
        self._mapping['deselect'].clicked.connect(self.uncheck_items)
        self._mapping['separated'].clicked.connect(self.check_seperated)
        self._mapping['grid'].clicked.connect(self.set_grid)
        self._mapping['log_scale'].stateChanged.connect(self.set_log)
        self._mapping['raw'].clicked.connect(self.check_raw)
        self._mapping['linestyle'].clicked.connect(self.change_linestyle)
        self._mapping['errorbar'].clicked.connect(self.change_errorbar)
        self._mapping['xaxis_scale'].currentIndexChanged.connect(
            self.something_changed)
        self.gridstate = 0
        self.linestyles = {0: '-', 1: '.', 2: '.-', }
        self.linestyle = 0
        self.errorbar = 0

    sig_plot = Signal(list)

    def check_first(self):
        self.model.check_first()

    def uncheck_items(self):
        self.model.itemChanged.disconnect()
        self.model.uncheck_items()
        self.model.itemChanged.connect(self.something_changed)
        self.something_changed()

    def check_seperated(self):
        self.model.itemChanged.disconnect()
        self.model.check_seperated()
        self.model.itemChanged.connect(self.something_changed)
        self.something_changed()

    def check_raw(self):
        self.model.itemChanged.disconnect()
        self.model.check_raw()
        self.model.itemChanged.connect(self.something_changed)
        self.something_changed()

    def something_changed(self):
        self.sig_plot.emit(self.model.get_checked_item_names())

    def change_errorbar(self):
        self.errorbar = (self.errorbar + 1) % 3
        self.something_changed()

    def change_linestyle(self):
        self.linestyle = (self.linestyle + 1) % 3
        self.something_changed()

    def get_datalist(self):
        return self.model.get_names()

    def set_line_and_grid_from_xml(self, gridstate, linestyle):
        self.gridstate = gridstate
        self.linestyle = linestyle
        self.something_changed()

    def set_datalist(self, datalist):
        self.model.itemChanged.disconnect()
        self.model.set_items(datalist)
        self.datalist.setModel(self.model)
        self.model.itemChanged.connect(self.something_changed)

    def clear_plot(self):
        if self.ax:
            self.ax.figure.clear()
        self.hasplot = False

    def get_xaxis(self):
        return self._mapping['xaxis_scale'].currentText()

    def plot(self):
        self.sig_plot.emit()

    def set_log(self, state):
        if state:
            self.ax.set_yscale('symlog')
        else:
            self.ax.set_yscale('linear')
        self.ax.figure.canvas.draw()

    def set_grid(self, dummy, draw=True):
        if draw:
            self.gridstate = (self.gridstate + 1) % 3
        if self.gridstate == 1:
            self.ax.xaxis.set_minor_locator(NullLocator())
            self.ax.grid(self.gridstate,
                         which='both',
                         zorder=-1000,
                         linestyle='--')
        elif self.gridstate == 2:
            self.ax.xaxis.set_minor_locator(AutoMinorLocator(5))
            self.ax.grid(self.gridstate,
                         which='both',
                         zorder=-1000,
                         linestyle='--')
        else:
            self.ax.xaxis.set_minor_locator(NullLocator())
            self.ax.grid(0)
        if draw:
            self.ax.figure.canvas.draw()

    def single_plot(self, x, y, yerr, label):
        if self.errorbar:
            self.ax.errorbar(x,
                             y,
                             yerr=yerr,
                             fmt=self.linestyles[self.linestyle],
                             label=label,
                             capsize=(self.errorbar - 1)*3)
        else:
            self.ax.plot(x, y, self.linestyles[self.linestyle], label=label)

    def create_plot(self, norm):
        if self.ax:
            self.ax.figure.clear()
        self.ax = self.static_canvas.figure.subplots()
        self.ax.set_title('Elastic Powder')
        self.ax.set_xlabel('2 theta (degree)')
        self.ax.set_ylabel('Intensity ({})'.format(norm))

    def finish_plot(self, xaxis):
        if self.model.get_checked_item_names():
            self.ax.legend()
        self.static_canvas.figure.tight_layout()
        if xaxis == 'q':
            self.ax.set_xlabel(r'$q (\AA^{-1})$')
        elif xaxis == 'd':
            self.ax.set_xlabel(r'$d (\AA)$')
        else:
            self.ax.set_xlabel('2 theta (degree)')

        self.set_grid(0, draw=False)
        self.set_log(self._mapping['log_scale'].checkState())
        self.hasplot = True

    def get_state(self):
        """
        returns a dictionary with the names of the widgets
        as keys and the values
        """
        state_dict = OrderedDict()

        for key, target_object in self._mapping.items():
            state = self.get_single_state(target_object)
            if state is not None: 
            ## pushbuttons for example are not defined in the get function
                state_dict[key] = state
        state_dict['linestyle'] = self.linestyle
        state_dict['gridstate'] = self.gridstate
        return state_dict

    def set_state(self, state_dict):
        """
        sets the gui state from a dictionary containing the shortnames
        of the widgets as keys and the values
        """
        self.gridstate = state_dict.get('gridstate', 0)
        self.linestyle = state_dict.get('linestyle', 0)
        for key, target_object in self._mapping.items():
            self.set_single_state(target_object,
                                  value=state_dict.get(key, None))
        self.something_changed()
        return
