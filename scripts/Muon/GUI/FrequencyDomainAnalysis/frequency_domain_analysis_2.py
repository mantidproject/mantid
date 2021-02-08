# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from qtpy import QtWidgets, QtCore, QT_VERSION
from distutils.version import LooseVersion

from mantid.kernel import ConfigServiceImpl

import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext, PlotMode
from Muon.GUI.Common.contexts.fitting_context import FittingContext
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext

from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.home_tab.home_tab_widget import HomeTabWidget
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_widget import TransformWidget
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget_new import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget_new import MaxEntWidget
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
from Muon.GUI.Common.phase_table_widget.phase_table_widget import PhaseTabWidget
from Muon.GUI.Common.results_tab_widget.results_tab_widget import ResultsTabWidget
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.Common.plot_widget.plot_widget import PlotWidget
from Muon.GUI.Common.plotting_dock_widget.plotting_dock_widget import PlottingDockWidget
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable

SUPPORTED_FACILITIES = ["ISIS", "SmuS"]
TAB_ORDER = ["Home", "Grouping", "Phase Table", "Transform", "Fitting", "Sequential Fitting", "Results"]


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


class FrequencyAnalysisGui(QtWidgets.QMainWindow):
    """
    The Frequency Domain Analaysis 2.0 interface.
    """

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(FrequencyAnalysisGui, self).__init__(parent)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

        try:
            check_facility()
        except AttributeError as error:
            self.warning_popup(error.args[0])

        # initialise the data storing classes of the interface
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(
            'Frequency Domain Data',
            self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_pair_context = MuonGroupPairContext(
            self.data_context.check_group_contains_valid_detectors)
        self.phase_context = PhaseTableContext()
        self.fitting_context = FittingContext()

        self.frequency_context = FrequencyContext()

        self.context = FrequencyDomainAnalysisContext(
            muon_data_context=self.data_context, muon_gui_context=self.gui_context,
            muon_group_context=self.group_pair_context, muon_phase_context=self.phase_context,
            fitting_context=self.fitting_context, frequency_context=self.frequency_context)

        # create the dockable widget
        self.fitting_tab = FittingTabWidget(self.context, self)
        self.plot_widget = PlotWidget(self.context, self.fitting_tab.fitting_tab_presenter.get_selected_fit_workspaces, parent=self)
        self.dockable_plot_widget_window = PlottingDockWidget(parent=self,
                                                              plotting_widget=self.plot_widget.view)
        self.dockable_plot_widget_window.setMinimumWidth(575)

        # Add dock widget to main Muon analysis window
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.dockable_plot_widget_window)
        # Need this line to stop the bug where the dock window snaps back to its original size after resizing.
        # 0 argument is arbitrary and has no effect on fit widget size
        # This is a qt bug reported at (https://bugreports.qt.io/browse/QTBUG-65592)
        if QT_VERSION >= LooseVersion("5.6"):
            self.resizeDocks({self.dockable_plot_widget_window}, {1}, QtCore.Qt.Horizontal)

        # construct all the widgets.
        self.load_widget = LoadWidget(self.loaded_data, self.context, self)
        self.grouping_tab_widget = GroupingTabWidget(self.context)
        self.home_tab = HomeTabWidget(self.context, self)
        self.phase_tab = PhaseTabWidget(self.context, self)
        self.transform = TransformWidget(
            self.context,
            FFTWidget,
            MaxEntWidget,
            parent=self)
        self.results_tab = ResultsTabWidget(
            self.context.fitting_context, self.context, self)

        self.setup_tabs()
        self.help_widget = HelpWidget(self.context.window_title)

        central_widget = QtWidgets.QWidget()
        vertical_layout = QtWidgets.QVBoxLayout()

        vertical_layout.addWidget(self.load_widget.load_widget_view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.disable_notifier = GenericObservable()
        self.disable_observer = GenericObserver(
            self.disable_notifier.notify_subscribers)
        self.enable_notifier = GenericObservable()
        self.enable_observer = GenericObserver(self.enable_notifier.notify_subscribers)
        self.setup_disable_notifier()
        self.setup_enable_notifier()

        self.setCentralWidget(central_widget)
        self.setWindowTitle(self.context.window_title)

        self.setup_load_observers()

        self.setup_gui_variable_observers()

        self.setup_grouping_changed_observers()

        self.setup_instrument_changed_notifier()

        self.setup_group_calculation_enable_notifier()

        self.setup_group_calculation_disabler_notifier()

        self.setup_on_load_enabler()

        self.setup_on_load_disabler()

        self.setup_phase_quad_changed_notifier()

        self.setup_phase_table_changed_notifier()
        self.setup_fitting_notifier()

        self.setup_on_recalculation_finished_notifier()

        self.transform.set_up_calculation_observers(
            self.fitting_tab.fitting_tab_presenter.enable_tab_observer,
            self.fitting_tab.fitting_tab_presenter.disable_tab_observer)
        self.transform.new_data_observer(
            self.transform_finished_observer)

        self.context.data_context.message_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.message_observer)

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab.home_tab_view, 'Home')
        self.tabs.addTabWithOrder(
            self.grouping_tab_widget.group_tab_view,
            'Grouping')
        self.tabs.addTabWithOrder(
            self.phase_tab.phase_table_view,
            'Phase Table')
        self.tabs.addTabWithOrder(self.transform.widget, 'Transform')
        self.tabs.addTabWithOrder(self.fitting_tab.fitting_tab_view, 'Fitting')
        self.tabs.addTabWithOrder(self.results_tab.results_tab_view, 'Results')
        self.update_plot_observer = GenericObserver(self.plot_widget.presenter.update_plot)
        self.transform_finished_observer = GenericObserverWithArgPassing(self.handle_transform_performed)
        self.tabs.set_slot_for_tab_changed(self.handle_tab_changed)

    def handle_tab_changed(self):
        index = self.tabs.currentIndex()
        if TAB_ORDER[index] in ["Home", "Grouping", "Phase Table"]:  # Plot all the selected data
            plot_mode = PlotMode.Data
        elif TAB_ORDER[index] in ["Fitting", "Transform"]:  # Plot the displayed workspace
            plot_mode = PlotMode.Fitting
        else:
            return

        self.plot_widget.presenter.handle_plot_mode_changed(plot_mode)

    def handle_transform_performed(self, new_data_workspace_name):
        self.fitting_tab.fitting_tab_presenter.handle_new_data_loaded()
        self.fitting_tab.fitting_tab_presenter.set_display_workspace(new_data_workspace_name)
        self.plot_widget.presenter.update_plot(autoscale=True)

    def setup_disable_notifier(self):

        self.disable_notifier.add_subscriber(self.home_tab.home_tab_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.load_widget.load_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.fitting_tab.fitting_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.phase_tab.phase_table_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.results_tab.results_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.transform.disable_observer)

        self.disable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.disable_tab_observer)

    def setup_enable_notifier(self):

        self.enable_notifier.add_subscriber(self.home_tab.home_tab_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.load_widget.load_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.fitting_tab.fitting_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.phase_tab.phase_table_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.results_tab.results_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.transform.enable_observer)

        self.enable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.enable_tab_observer)

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.home_tab.home_tab_widget.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.transform.LoadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.run_change_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.disable_tab_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.plot_widget.presenter.new_data_loaded_observer)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        self.context.gui_context.gui_variable_non_calulation_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.plot_widget.presenter.rebin_options_set_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.plot_widget.presenter.added_group_or_pair_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.transform.GroupPairObserver)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.transform.GroupPairObserver)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.plot_widget.presenter.added_group_or_pair_observer)

        self.plot_widget.presenter.plot_type_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_plot_type_observer)

        self.fitting_tab.fitting_tab_presenter.selected_single_fit_notifier.add_subscriber(
            self.plot_widget.presenter.plot_selected_fit_observer)

        self.phase_tab.phase_table_presenter.selected_phasequad_changed_notifier.add_subscriber(
            self.plot_widget.presenter.added_group_or_pair_observer)

        self.phase_tab.phase_table_presenter.selected_phasequad_changed_notifier.add_subscriber(
            self.transform.GroupPairObserver)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.transform.GroupPairObserver)

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
            self.transform.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.instrument_changed_observer)

    def setup_group_calculation_enable_notifier(self):
        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.fitting_tab.fitting_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.phase_tab.phase_table_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

    def setup_group_calculation_disabler_notifier(self):
        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.fitting_tab.fitting_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.phase_tab.phase_table_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.transform.enable_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.transform.disable_observer)

    def setup_on_recalculation_finished_notifier(self):
        self.grouping_tab_widget.group_tab_presenter.calculation_finished_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.input_workspace_observer)

        self.grouping_tab_widget.group_tab_presenter.calculation_finished_notifier.add_subscriber(
            self.update_plot_observer)

    def setup_phase_quad_changed_notifier(self):
        self.phase_tab.phase_table_presenter.phase_quad_calculation_complete_notifier.add_subscriber(
            self.transform.phase_quad_observer)

    def setup_phase_table_changed_notifier(self):
        self.phase_tab.phase_table_presenter.phase_table_calculation_complete_notifier.add_subscriber(
            self.transform._maxent._presenter.phase_table_observer)

    def setup_fitting_notifier(self):
        """Connect fitting and results tabs to inform of new fits"""
        self.fitting_context.new_fit_results_notifier.add_subscriber(
            self.results_tab.results_tab_presenter.new_fit_performed_observer)

        self.fitting_context.plot_guess_notifier.add_subscriber(self.plot_widget.presenter.plot_guess_observer)

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        self.context.ads_observer.unsubscribe()
        self.context.ads_observer = None
        super(FrequencyAnalysisGui, self).closeEvent(event)
