# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import PyQt4.QtGui as QtGui
import PyQt4.QtCore as QtCore
from mantid.kernel import ConfigServiceImpl
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget

from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget

from Muon.GUI.Common.home_tab.home_tab_widget import HomeTabWidget

SUPPORTED_FACILITIES = ["ISIS", "SmuS"]


def check_facility():
    """
    Get the currently set facility and check if it is in the list
    of supported facilities, raising an AttributeError if not.
    """
    current_facility = ConfigServiceImpl.Instance().getFacility().name()
    if current_facility not in SUPPORTED_FACILITIES:
        raise AttributeError("Your facility {} is not supported by MuonAnalysis 2.0, so you"
                             "will not be able to load any files. \n \n"
                             "Supported facilities are :"
                             + "\n - ".join(SUPPORTED_FACILITIES))


class MuonAnalysisGui(QtGui.QMainWindow):
    """
    The Muon Analaysis 2.0 interface.
    """
    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(MuonAnalysisGui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        try:
            check_facility()
        except AttributeError as error:
            self.warning_popup(error.args[0])

        # initialise the data storing classes of the interface
        self.loaded_data = MuonLoadData()
        self.context = MuonDataContext(load_data=self.loaded_data)

        # construct all the widgets.
        self.load_widget = LoadWidget(self.loaded_data, self.context, self)
        self.grouping_tab_widget = GroupingTabWidget(self.context)
        self.home_tab = HomeTabWidget(self.context, self)

        self.setup_tabs()
        self.help_widget = HelpWidget()

        central_widget = QtGui.QWidget()
        vertical_layout = QtGui.QVBoxLayout()

        vertical_layout.addWidget(self.load_widget.load_widget_view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle("Muon Analysis version 2")

        self.home_tab.group_widget.pairAlphaNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        self.context.instrumentNotifier.add_subscriber(
            self.home_tab.home_tab_widget.instrumentObserver)

        self.context.instrumentNotifier.add_subscriber(
            self.load_widget.load_widget.instrumentObserver)

        self.context.instrumentNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.instrumentObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.home_tab.home_tab_widget.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.context.message_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.message_observer)
        self.context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab.home_tab_view, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view, 'Grouping')

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        super(MuonAnalysisGui, self).closeEvent(event)
