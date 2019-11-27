# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Reduction GUI for DNS Instrument at MLZ
"""
from __future__ import (absolute_import, division, print_function)
import webbrowser

from qtpy.QtWidgets import QMainWindow
from qtpy.QtCore import Signal

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui
from mantidqt.interfacemanager import InterfaceManager

from DNSReduction.main_presenter import DNSReductionGUI_presenter


class DNSReductionGUI_view(QMainWindow):
    """
    Main View for DNS reduction gui
    """
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        # load main ui file for gui
        self.ui = load_ui(__file__,
                          'dns_gui_main_reduced_menu.ui',
                          baseinstance=self)
        ### connect menu signals
        self.ui.actionQuit.triggered.connect(self.close)
        self.ui.actionSave_as.triggered.connect(self.save_as_triggered)
        self.ui.actionOpen.triggered.connect(self.open_triggered)
        self.ui.actionMantid_help.triggered.connect(self.help_button_clicked)
        self.ui.actionDNS_website.triggered.connect(self.open_dns_webpage)
        ### connect mode swithcing signals
        self.modus_mapping = {
            self.ui.actionSimulation: 'simulation',
            self.ui.actionPowder_elastic: 'powder_elastic',
            self.ui.actionPowder_TOF: 'powder_tof',
            #self.ui.actionSingle_crystal_elastic : 'sc_elastic',
            #self.ui.actionSingle_crystal_TOF     : 'sc_tof',
        }
        self.modus_titles = {
            'simulation': 'DNS Reduction - Simulation',
            'powder_elastic': 'DNS Reduction - Powder elastic',
            'powder_tof': 'DNS Reduction - Powder TOF',
            #'sc_elastic'       : 'DNS Reduction - Single Crystal elastic',
            #'sc_tof'           : 'DNS Reduction - Single Crystal TOF',
        }
        for key in self.modus_mapping:
            key.triggered.connect(self.modus_change)

        self.subviews = []
        self.main_presenter = DNSReductionGUI_presenter(view=self)
        self.last_index = 0

        ## Connect Signals
        self.ui.tabWidget.currentChanged.connect(self._tab_changed)
        return


## Signals

    sig_tab_changed = Signal(int, int)
    sig_save_as_triggered = Signal()
    sig_open_triggered = Signal()
    sig_modus_change = Signal(str)

    def _add_tab(self, newtab, position=-1):
        self.ui.tabWidget.insertTab(position, newtab, newtab.name)

    def _clear_tabs(self):
        self.ui.tabWidget.clear()

    def _remove_tab(self, tab):
        index = self.ui.tabWidget.indexOf(tab)
        if index != -1:
            self.ui.tabWidget.removeTab(index)

    def _tab_changed(self, index):
        self.sig_tab_changed.emit(self.last_index, index)
        self.last_index = index

    def add_subview(self, subview):
        if subview.has_tab:
            self.subviews.append(subview)
            self._add_tab(subview)

    def clear_subviews(self):
        self.subviews = []
        self._clear_tabs()

    def get_view_for_tabindex(self, tabindex):
        if tabindex <= len(self.subviews):
            return self.subviews[tabindex]
        return None

    def help_button_clicked(self):
		InterfaceManager().showCustomInterfaceHelp('DNS Reduction')

    def modus_change(self):
        self.ui.tabWidget.currentChanged.disconnect(self._tab_changed)
        self.last_index = 0
        modus = self.modus_mapping[self.sender()]
        self.setWindowTitle(self.modus_titles[modus])
        self.sig_modus_change.emit(modus)
        self.ui.tabWidget.currentChanged.connect(self._tab_changed)

    def open_dns_webpage(self):
        webbrowser.open(
            'https://www.mlz-garching.de/instrumente-und-labore'\
                '/spektroskopie/dns.html', new=1, autoraise=True)

    def open_triggered(self):
        self.sig_open_triggered.emit()

    def save_as_triggered(self):
        self.sig_save_as_triggered.emit()

    def show_statusmessage(self, message='', time=10, clear=False):
        oldmessage = self.ui.statusbar.currentMessage()
        if oldmessage and not clear:
            message = " AND ".join((message, oldmessage))
        self.ui.statusbar.showMessage(message, time * 1000)
