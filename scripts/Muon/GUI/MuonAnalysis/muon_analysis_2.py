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
from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext
from Muon.GUI.Common.contexts.data_analysis_context import DataAnalysisContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext
from Muon.GUI.Common.contexts.results_context import ResultsContext
from Muon.GUI.Common.contexts.fitting_contexts.model_fitting_context import ModelFittingContext
from Muon.GUI.Common.contexts.fitting_contexts.tf_asymmetry_fitting_context import TFAsymmetryFittingContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.contexts.plot_pane_context import PlotPanesContext
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget import GroupingTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.home_tab.home_tab_widget import HomeTabWidget
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.corrections_tab_widget.corrections_tab_widget import CorrectionsTabWidget
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget
from Muon.GUI.Common.model_fitting_tab_widget.model_fitting_tab_widget import ModelFittingTabWidget
from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_widget import SeqFittingTabWidget
from Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
from Muon.GUI.Common.phase_table_widget.phase_table_widget import PhaseTabWidget
from Muon.GUI.Common.results_tab_widget.results_tab_widget import ResultsTabWidget
from Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget
from Muon.GUI.Common.plotting_dock_widget.plotting_dock_widget import PlottingDockWidget
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable


SUPPORTED_FACILITIES = ["ISIS", "SmuS"]
TAB_ORDER = ["Home", "Grouping", "Corrections", "Phase Table", "Fitting", "Sequential Fitting", "Results",
             "Model Fitting"]


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
    The Muon Analysis 2.0 interface.
    """

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None, window_flags=None):
        super(MuonAnalysisGui, self).__init__(parent)
        if window_flags:
            self.setWindowFlags(window_flags)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setObjectName("MuonAnalysis2")
        self.current_tab = ''

        try:
            check_facility()
        except AttributeError as error:
            self.warning_popup(error.args[0])

        # initialise the data storing classes of the interface
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext('Muon Data', self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_pair_context = MuonGroupPairContext(
            self.data_context.check_group_contains_valid_detectors)
        self.corrections_context = CorrectionsContext(self.loaded_data)
        self.phase_context = PhaseTableContext()
        self.fitting_context = TFAsymmetryFittingContext(allow_double_pulse_fitting=True)
        self.results_context = ResultsContext()
        self.model_fitting_context = ModelFittingContext(allow_double_pulse_fitting=False)
        self.plot_panes_context = PlotPanesContext()
        self.context = DataAnalysisContext(muon_data_context=self.data_context,
                                           muon_gui_context=self.gui_context,
                                           muon_group_context=self.group_pair_context,
                                           corrections_context=self.corrections_context,
                                           fitting_context=self.fitting_context,
                                           results_context=self.results_context,
                                           model_fitting_context=self.model_fitting_context,
                                           muon_phase_context=self.phase_context,
                                           plot_panes_context=self.plot_panes_context)

        # create the Dockable plot widget
        self.fitting_tab = FittingTabWidget(self.context, self)
        self.plot_widget = MuonAnalysisPlotWidget(self.context, parent=self)
        self.dockable_plot_widget_window = PlottingDockWidget(parent=self,
                                                              plotting_widget=self.plot_widget.view)
        self.dockable_plot_widget_window.setMinimumWidth(800)

        # Add dock widget to main Muon analysis window
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.dockable_plot_widget_window)
        # Need this line to stop the bug where the dock window snaps back to its original size after resizing.
        # 0 argument is arbitrary and has no effect on fit widget size
        # This is a qt bug reported at (https://bugreports.qt.io/browse/QTBUG-65592)
        if QT_VERSION >= LooseVersion("5.6"):
            self.resizeDocks({self.dockable_plot_widget_window}, {1}, QtCore.Qt.Horizontal)

        self.disable_notifier = GenericObservable()
        self.disable_observer = GenericObserver(self.disable_notifier.notify_subscribers)
        self.enable_notifier = GenericObservable()
        self.enable_observer = GenericObserver(self.enable_notifier.notify_subscribers)

        # set up other widgets
        self.load_widget = LoadWidget(self.loaded_data, self.context, self)
        self.home_tab = HomeTabWidget(self.context, self)
        self.grouping_tab_widget = GroupingTabWidget(self.context, parent)
        self.corrections_tab = CorrectionsTabWidget(self.context, self)
        self.phase_tab = PhaseTabWidget(self.context, self)
        self.seq_fitting_tab = SeqFittingTabWidget(self.context, self.fitting_tab.fitting_tab_model, self)
        self.results_tab = ResultsTabWidget(self.context.fitting_context, self.context, self)
        self.model_fitting_tab = ModelFittingTabWidget(self.context, self)

        self.setup_tabs()
        self.help_widget = HelpWidget("Muon Analysis 2")

        central_widget = QtWidgets.QWidget()
        vertical_layout = QtWidgets.QVBoxLayout()
        vertical_layout.addWidget(self.load_widget.load_widget_view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle(self.context.window_title)

        self.setup_load_observers()

        self.setup_gui_variable_observers()

        self.setup_grouping_changed_observers()

        self.setup_corrections_changed_observers()

        self.setup_instrument_changed_notifier()

        self.setup_group_calculation_enable_notifier()

        self.setup_group_calculation_disabler_notifier()

        self.setup_on_load_enabler()

        self.setup_on_load_disabler()

        self.setup_phase_quad_changed_notifier()

        self.setup_phase_table_changed_notifier()

        self.setup_fitting_notifier()

        self.setup_counts_calculation_finished_notifier()

        self.setup_asymmetry_pair_and_diff_calculations_finished_notifier()

        self.setup_results_notifier()

        self.context.data_context.message_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.message_observer)

        self.setup_disable_notifier()

        self.setup_enable_notifier()

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab.home_tab_view, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view, 'Grouping')
        self.tabs.addTabWithOrder(self.corrections_tab.corrections_tab_view, "Corrections")
        self.tabs.addTabWithOrder(self.phase_tab.phase_table_view, 'Phase Table')
        self.tabs.addTabWithOrder(self.fitting_tab.fitting_tab_view, 'Fitting')
        self.tabs.addTabWithOrder(self.seq_fitting_tab.seq_fitting_tab_view, 'Sequential Fitting')
        self.tabs.addTabWithOrder(self.results_tab.results_tab_view, 'Results')
        self.tabs.addTabWithOrder(self.model_fitting_tab.model_fitting_tab_view, 'Model Fitting')
        self.tabs.set_slot_for_tab_changed(self.handle_tab_changed)
        self.tabs.setElideMode(QtCore.Qt.ElideNone)
        self.tabs.setUsesScrollButtons(False)

    def handle_tab_changed(self):
        index = self.tabs.currentIndex()
        # the plot mode indicies are from the order the plots are stored
        if TAB_ORDER[index] in ["Home", "Grouping", "Corrections", "Phase Table"]:  # Plot all the selected data
            plot_mode = self.plot_widget.data_index
        # Plot the displayed workspace
        elif TAB_ORDER[index] in ["Fitting", "Sequential Fitting"]:
            plot_mode = self.plot_widget.fit_index
        elif TAB_ORDER[index] in ["Model Fitting"]:
            plot_mode = self.plot_widget.model_fit_index
        else:
            return
        self.plot_widget.set_plot_view(plot_mode)

    def set_tab_warning(self, tab_name: str, message: str):
        """Sets a warning message as the tooltip of the provided tab."""
        self.tabs.set_tab_warning(TAB_ORDER.index(tab_name), message)

    def setup_disable_notifier(self):

        self.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.corrections_tab.corrections_tab_view.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.load_widget.load_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.fitting_tab.fitting_tab_view.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.model_fitting_tab.model_fitting_tab_view.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.phase_tab.phase_table_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.results_tab.results_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.seq_fitting_tab.seq_fitting_tab_presenter.disable_tab_observer)

    def setup_enable_notifier(self):

        self.enable_notifier.add_subscriber(self.home_tab.home_tab_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.corrections_tab.corrections_tab_view.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.load_widget.load_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.fitting_tab.fitting_tab_view.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.model_fitting_tab.model_fitting_tab_view.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.phase_tab.phase_table_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.results_tab.results_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.seq_fitting_tab.seq_fitting_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.enable_tab_observer)

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.home_tab.home_tab_widget.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.corrections_tab.corrections_tab_presenter.load_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.run_change_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.fitting_tab.fitting_tab_view.disable_tab_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.disable_tab_observer)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.plot_widget.raw_mode.new_data_observer)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        for observer in self.plot_widget.rebin_options_set_observers:
            self.context.gui_context.gui_variables_notifier.add_subscriber(observer)

        self.context.gui_context.gui_variable_non_calulation_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.gui_context_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.plot_widget.data_mode.added_group_or_pair_observer)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.plot_widget.data_mode.added_group_or_pair_observer)

        self.grouping_tab_widget.pairing_table_widget.selected_pair_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer)

        # differences
        self.grouping_tab_widget.diff_table.add_subscribers(
            [self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer,
             self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer,
             self.plot_widget.data_mode.added_group_or_pair_observer])

        # phase table set up
        self.phase_tab.phase_table_presenter.selected_phasequad_changed_notifier.add_subscriber(
            self.plot_widget.data_mode.added_group_or_pair_observer)

        self.phase_tab.phase_table_presenter.selected_phasequad_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.selected_group_pair_observer)

        self.phase_tab.phase_table_presenter.selected_phasequad_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer)

        self.fitting_tab.fitting_tab_presenter.fit_function_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.fit_function_updated_observer)

        self.fitting_tab.fitting_tab_presenter.fit_parameter_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.fit_parameter_updated_observer)

        self.seq_fitting_tab.seq_fitting_tab_presenter.fit_parameter_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.fit_parameter_updated_observer)

        self.seq_fitting_tab.seq_fitting_tab_presenter.sequential_fit_finished_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.sequential_fit_finished_observer)

        self.fitting_tab.fitting_tab_presenter.fitting_mode_changed_notifier.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.fit_type_changed_observer)

        self.fitting_tab.fitting_tab_presenter.simultaneous_fit_by_specifier_changed.add_subscriber(
            self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer)

        self.fitting_tab.fitting_tab_presenter.selected_fit_results_changed.add_subscriber(
            self.plot_widget.fit_mode.plot_selected_fit_observer)

        self.seq_fitting_tab.seq_fitting_tab_presenter.selected_sequential_fit_notifier.add_subscriber(
            self.plot_widget.fit_mode.plot_selected_fit_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.selected_fit_results_changed.add_subscriber(
            self.plot_widget.model_fit_mode.plot_selected_fit_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.update_plot_x_range_notifier.add_subscriber(
            self.plot_widget.model_fit_mode.update_x_range_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.update_override_tick_labels_notifier.add_subscriber(
            self.plot_widget.model_fit_mode.update_override_tick_labels_observer)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.corrections_tab.corrections_tab_presenter.group_change_observer)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.group_change_observer)

    def setup_corrections_changed_observers(self):
        self.corrections_tab.corrections_tab_presenter.perform_corrections_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

    def setup_instrument_changed_notifier(self):
        self.context.data_context.instrumentNotifier.add_subscriber(
            self.home_tab.home_tab_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.load_widget.load_widget.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.instrumentObserver)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.corrections_tab.corrections_tab_presenter.instrument_changed_observer)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.instrument_changed_observer)

        self.context.data_context.instrumentNotifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.instrument_changed_observer)

    def setup_group_calculation_enable_notifier(self):

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.corrections_tab.corrections_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.fitting_tab.fitting_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.phase_tab.phase_table_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

    def setup_group_calculation_disabler_notifier(self):

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.corrections_tab.corrections_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.fitting_tab.fitting_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.phase_tab.phase_table_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_view.enable_tab_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.home_tab.home_tab_widget.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_view.disable_tab_observer)

    def setup_counts_calculation_finished_notifier(self):
        self.grouping_tab_widget.group_tab_presenter.counts_calculation_finished_notifier.add_subscriber(
            self.corrections_tab.corrections_tab_presenter.pre_process_and_counts_calculated_observer)

    def setup_asymmetry_pair_and_diff_calculations_finished_notifier(self):
        self.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.\
            add_subscriber(self.fitting_tab.fitting_tab_presenter.input_workspace_observer)

        self.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.\
            add_subscriber(self.seq_fitting_tab.seq_fitting_tab_presenter.selected_workspaces_observer)

        for observer in self.plot_widget.data_changed_observers:
            self.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.\
                add_subscriber(observer)
            self.phase_tab.phase_table_presenter.calculation_finished_notifier.add_subscriber(observer)

    def setup_phase_quad_changed_notifier(self):
        pass

    def setup_phase_table_changed_notifier(self):
        pass

    def setup_fitting_notifier(self):
        """Connect fitting and results tabs to inform of new fits"""
        self.fitting_context.new_fit_results_notifier.add_subscriber(
            self.results_tab.results_tab_presenter.new_fit_performed_observer)

        self.fitting_tab.fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber(
            self.plot_widget.fit_mode.remove_plot_guess_observer)

        self.fitting_tab.fitting_tab_presenter.update_plot_guess_notifier.add_subscriber(
            self.plot_widget.fit_mode.update_plot_guess_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber(
            self.plot_widget.model_fit_mode.remove_plot_guess_observer)

        self.model_fitting_tab.model_fitting_tab_presenter.update_plot_guess_notifier.add_subscriber(
            self.plot_widget.model_fit_mode.update_plot_guess_observer)

    def setup_results_notifier(self):
        """Connect results tab to the model fitting tab."""
        self.results_tab.results_tab_presenter.results_table_created_notifier.add_subscriber(
            self.model_fitting_tab.model_fitting_tab_presenter.results_table_created_observer)

    def closeEvent(self, event):
        self.removeDockWidget(self.dockable_plot_widget_window)
        self.tabs.closeEvent(event)
        self.context.ads_observer.unsubscribe()
        self.grouping_tab_widget.group_tab_presenter.closePeriodInfoWidget()
        super(MuonAnalysisGui, self).closeEvent(event)
