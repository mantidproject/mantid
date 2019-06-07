# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore

from mantid.kernel import ConfigServiceImpl

import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext
from Muon.GUI.Common.contexts.fitting_context import FittingContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.home_tab.home_tab_widget import HomeTabWidget
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
from Muon.GUI.Common.phase_table_widget.phase_table_widget import PhaseTabWidget
from Muon.GUI.Common.results_tab_widget.results_tab_widget import ResultsTabWidget

SUPPORTED_FACILITIES = ["ISIS", "SmuS"]


def check_facility():
    """
    Get the currently set facility and check if it is in the list
    of supported facilities, raising an AttributeError if not.
    """
    current_facility = ConfigServiceImpl.Instance().getFacility().name()
    if current_facility not in SUPPORTED_FACILITIES:
        raise AttributeError(
            "Your facility {} is not supported by MuonAnalysis 2.0, so you"
            "will not be able to load any files. \n \n"
            "Supported facilities are :" + "\n - ".join(SUPPORTED_FACILITIES))


class MuonAnalysisGui(QtWidgets.QMainWindow):
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
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_pair_context = MuonGroupPairContext(
            self.data_context.check_group_contains_valid_detectors)
        self.phase_context = PhaseTableContext()
        self.fitting_context = FittingContext()

        self.context = MuonContext(muon_data_context=self.data_context,
                                   muon_gui_context=self.gui_context,
                                   muon_group_context=self.group_pair_context,
                                   muon_phase_context=self.phase_context,
                                   fitting_context=self.fitting_context)

        # construct all the widgets.
        self.load_widget = LoadWidget(self.loaded_data, self.context, self)
        self.grouping_tab_widget = GroupingTabWidget(self.context)
        self.home_tab = HomeTabWidget(self.context, self)
        self.phase_tab = PhaseTabWidget(self.context, self)
        self.fitting_tab = FittingTabWidget(self.context, self)
        self.results_tab = ResultsTabWidget(self.context.fitting_context, self)

        self.setup_tabs()
        self.help_widget = HelpWidget("Muon Analysis 2")

        central_widget = QtWidgets.QWidget()
        vertical_layout = QtWidgets.QVBoxLayout()

        vertical_layout.addWidget(self.load_widget.load_widget_view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle("Muon Analysis")

        self.setup_load_observers()

        self.setup_gui_variable_observers()

        self.setup_alpha_recalculated_observers()

        self.setup_grouping_changed_observers()

        self.setup_instrument_changed_notifier()

        self.setup_group_calculation_enable_notifer()

        self.setup_group_calculation_disabler_notifer()

        self.setup_on_load_enabler()

        self.setup_on_load_disabler()

        self.setup_phase_quad_changed_notifer()

        self.setup_phase_table_changed_notifier()

        self.setup_fitting_notifier()

        self.context.data_context.message_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.message_observer)

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab.home_tab_view, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view,
                                  'Grouping')
        self.tabs.addTabWithOrder(self.phase_tab.phase_table_view,
                                  'Phase Table')
        self.tabs.addTabWithOrder(self.fitting_tab.fitting_tab_view, 'Fitting')
        self.tabs.addTabWithOrder(self.results_tab.results_tab_view, 'Results')

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.home_tab.home_tab_widget.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.run_change_observer)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer
        )

        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        self.context.gui_context.gui_variable_non_calulation_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        self.home_tab.group_widget.selected_group_pair_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

    def setup_alpha_recalculated_observers(self):
        self.home_tab.group_widget.pairAlphaNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.group_change_observer)

    def setup_instrument_changed_notifier(self):
        self.context.data_context.instrumentNotifier.add_subscriber(
            self.home_tab.home_tab_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.load_widget.load_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.instrument_changed_observer)

    def setup_group_calculation_enable_notifer(self):
        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.enable_tab_observer)

    def setup_group_calculation_disabler_notifer(self):
        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.load_widget.load_widget.disable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.disable_tab_observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.enable_tab_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.disable_tab_observer)

    def setup_on_recalulation_finished_notifer(self):
        self.grouping_tab_widget.group_tab_presenter.calculation_finished_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.input_workspace_observer)

    def setup_phase_quad_changed_notifer(self):
        pass

    def setup_phase_table_changed_notifier(self):
        pass

    def setup_fitting_notifier(self):
        """Connect fitting and results tabs to inform of new fits"""
        self.fitting_context.new_fit_notifier.add_subscriber(
            self.results_tab.results_tab_presenter.new_fit_performed_observer)

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        super(MuonAnalysisGui, self).closeEvent(event)
