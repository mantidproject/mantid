# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Reduction GUI for DNS Instrument at MLZ
"""

import webbrowser

from mantidqt.utils.qt import load_ui
from mantidqt.gui_helper import show_interface_help

from qtpy.QtCore import Signal
from qtpy.QtWidgets import QMainWindow
from qtpy.QtCore import QProcess


class DNSReductionGUIView(QMainWindow):
    # pylint: disable=too-many-instance-attributes
    """
    Main View for DNS reduction gui
    """

    def __init__(self, parent=None, app=None, within_mantid=None):
        QMainWindow.__init__(self, parent=None)
        self.parent = parent
        self.app = app
        self.within_mantid = within_mantid
        # load main ui file for gui
        self.ui = load_ui(__file__,
                          'dns_gui_main_reduced_menu.ui',
                          baseinstance=self)
        self.subview_menus = []
        self.last_index = 0
        # connect menu signals
        self.ui.actionQuit.triggered.connect(self.close)
        self.ui.actionSave_as.triggered.connect(self._save_as_triggered)
        self.ui.actionSave.triggered.connect(self._save_triggered)
        self.ui.actionOpen.triggered.connect(self._open_triggered)
        self.ui.actionMantid_help.triggered.connect(self._help_button_clicked)
        self.ui.actionDNS_website.triggered.connect(self._open_dns_webpage)
        # connect mode switching signals
        self.modus_mapping = {
            # self.ui.actionSimulation: 'simulation',
            # self.ui.actionPowder_elastic: 'powder_elastic',
            self.ui.actionPowder_TOF: 'powder_tof',
            # self.ui.actionSingle_crystal_elastic: 'sc_elastic',
            # self.ui.actionSingle_crystal_TOF: 'sc_tof',
        }
        self.modus_titles = {
            # 'simulation': 'DNS Reduction - Simulation',
            # 'powder_elastic': 'DNS Reduction - Powder elastic',
            'powder_tof': 'DNS Reduction - Powder TOF',
            # 'sc_elastic': 'DNS Reduction - Single Crystal elastic',
            # 'sc_tof': 'DNS Reduction - Single Crystal TOF',
        }
        for key in self.modus_mapping:
            key.triggered.connect(self._modus_change)
        self.menu = self.ui.menubar
        self.subviews = []
        self.last_index = 0
        # Connect Signals
        self.ui.tabWidget.currentChanged.connect(self._tab_changed)

    # Signals
    sig_tab_changed = Signal(int, int)
    sig_save_as_triggered = Signal()
    sig_save_triggered = Signal()
    sig_open_triggered = Signal()
    sig_modus_change = Signal(str)

    def _add_tab(self, newtab, position=-1):
        self.ui.tabWidget.insertTab(position, newtab, newtab.NAME)

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
        if subview.HAS_TAB:
            subview.setVisible(True)
            self.subviews.append(subview)
            self._add_tab(subview)

    def clear_subviews(self):
        for subview in self.subviews:
            subview.setVisible(False)
        self.subviews = []
        self._clear_tabs()

    def get_view_for_tabindex(self, tabindex):
        if tabindex <= len(self.subviews):
            return self.subviews[tabindex]
        return None

    def _help_button_clicked(self):
        show_interface_help("direct/dns_powder_tof/DNS Reduction",
                            QProcess(self))

    def add_submenu(self, subview):
        for menu in subview.menues:
            submenu = self.menu.insertMenu(self.ui.menuHelp.menuAction(), menu)
            self.subview_menus.append(submenu)

    def clear_submenues(self):
        for submenu in self.subview_menus:
            self.menu.removeAction(submenu)

    def _modus_change(self):
        self.ui.tabWidget.currentChanged.disconnect(self._tab_changed)
        self.last_index = 0
        modus = self.modus_mapping[self.sender()]
        self.setWindowTitle(self.modus_titles[modus])
        self.sig_modus_change.emit(modus)
        self.ui.tabWidget.currentChanged.connect(self._tab_changed)

    @staticmethod
    def _open_dns_webpage():
        webbrowser.open('https://mlz-garching.de/dns', new=1, autoraise=True)

    def _open_triggered(self):
        self.sig_open_triggered.emit()

    def _save_as_triggered(self):
        self.sig_save_as_triggered.emit()

    def _save_triggered(self):
        self.sig_save_triggered.emit()

    def show_statusmessage(self, message='', time=10, clear=False):
        oldmessage = self.ui.statusbar.currentMessage()
        if oldmessage and not clear:
            message = " AND ".join((message, oldmessage))
        self.ui.statusbar.showMessage(message, time * 1000)

    def switch_to_plot_tab(self):
        for i, subview in enumerate(self.subviews):
            if 'Plot' in subview.NAME:
                self.ui.tabWidget.setCurrentIndex(i)
                self._tab_changed(i)
                break
