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

from Muon.GUI.Common.muon_context import MuonContext
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget

from Muon.GUI.Common.home_tab.home_tab_widget import HomeTabWidget

from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_widget import TransformWidget
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext
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


class FrequencyAnalysisGui(QtGui.QMainWindow):

    """
    The Frequency Domain Analaysis 2.0 interface.
    """
    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(FrequencyAnalysisGui, self).__init__(parent)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        try:
            check_facility()
        except AttributeError as error:
            self.warning_popup(error.args[0])

        # initialise the data storing classes of the interface
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()

        self.group_pair_context = MuonGroupPairContext()

        self.context = MuonContext(muon_data_context=self.data_context, muon_gui_context=self.gui_context,
                                   muon_group_context=self.group_pair_context)

        # construct all the widgets.
        self.load_widget = LoadWidget(self.loaded_data, self.context, self)
        self.grouping_tab_widget = GroupingTabWidget(self.context)
        self.home_tab = HomeTabWidget(self.context, self)
        freq_context = FrequencyContext(self.context)
        self.transform = TransformWidget(load=freq_context, parent=self)

        self.setup_tabs()
        self.help_widget = HelpWidget()

        central_widget = QtGui.QWidget()
        vertical_layout = QtGui.QVBoxLayout()

        vertical_layout.addWidget(self.load_widget.load_widget_view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle("Frequency Domain Analysis")

        self.setup_load_observers()

        self.setup_gui_variable_observers()

        self.setup_alpha_recalculated_observers()

        self.setup_grouping_changed_observers()

        self.setup_instrument_changed_notifier()

        self.setup_group_calculation_enable_notifer()

        self.setup_group_calculation_disabler_notifer()

        self.setup_on_load_enabler()

        self.setup_on_load_disabler()

        self.context.data_context.message_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.message_observer)

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab.home_tab_view, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view, 'Grouping')
        self.tabs.addTabWithOrder(self.transform.widget, 'Transform')

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.home_tab.home_tab_widget.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        # self.load_widget.load_widget.loadNotifier.add_subscriber(
        #     self.transform.LoadObserver)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

    def setup_alpha_recalculated_observers(self):
        self.home_tab.group_widget.pairAlphaNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        #
        # self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
        #     self.transform.GroupPairObserver)

    def setup_instrument_changed_notifier(self):
        self.context.data_context.instrumentNotifier.add_subscriber(
            self.home_tab.home_tab_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.load_widget.load_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.instrumentObserver)
        #
        # self.context.data_context.instrumentNotifier.add_subscriber(
        #     self.transform.instrumentObserver)

    def setup_group_calculation_enable_notifer(self):
        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.enable_observer)

    def setup_group_calculation_disabler_notifer(self):
        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.disable_observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

        # self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
        #     self.transform.enable_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

        # self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
        #     self.transform.disable_observer)

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        super(FrequencyAnalysisGui, self).closeEvent(event)
