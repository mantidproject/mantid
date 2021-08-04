# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from distutils.version import LooseVersion

from qtpy import QtWidgets, QtCore, QT_VERSION

import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import GeneralFittingContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.contexts.plot_pane_context import PlotPanesContext
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.plotting_dock_widget.plotting_dock_widget import PlottingDockWidget
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_widget import EAAutoTabWidget
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_widget import EAGroupingTabWidget
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab import EAFittingTabWidget
from Muon.GUI.ElementalAnalysis2.load_widget.load_widget import LoadWidget
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plot_widget import EAPlotWidget
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable, GenericObserverWithArgPassing

TAB_ORDER = ["Home", "Grouping", "Fitting", "Automatic"]


class ElementalAnalysisGui(QtWidgets.QMainWindow):
    """
    The Elemental Analysis 2.0 interface.
    """

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def __init__(self, parent=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setObjectName("ElementalAnalysis2")

        # setup error notifier and observer for context and group context
        self.error_notifier = GenericObservable()
        self.error_observer = GenericObserverWithArgPassing(self.warning_popup)
        self.error_notifier.add_subscriber(self.error_observer)

        self.loaded_data = MuonLoadData()
        self.data_context = DataContext(self.loaded_data)
        self.group_context = EAGroupContext(self.data_context.check_group_contains_valid_detectors, self.error_notifier)
        self.gui_context = MuonGuiContext()
        self.plot_panes_context = PlotPanesContext()
        self.fitting_context = GeneralFittingContext()
        self.context = ElementalAnalysisContext(self.data_context, self.group_context, self.gui_context,
                                                self.plot_panes_context, self.fitting_context, self.error_notifier)

        self.current_tab = ''

        self.plot_widget = EAPlotWidget(self.context, parent=self)
        self.dockable_plot_widget_window = PlottingDockWidget(parent=self,
                                                              plotting_widget=self.plot_widget.view)
        self.dockable_plot_widget_window.setMinimumWidth(800)

        # Add dock widget to main Elemental analysis window
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.dockable_plot_widget_window)
        # Need this line to stop the bug where the dock window snaps back to its original size after resizing.
        # 0 argument is arbitrary and has no effect on fit widget size
        # This is a qt bug reported at (https://bugreports.qt.io/browse/QTBUG-65592)
        if QT_VERSION >= LooseVersion("5.6"):
            self.resizeDocks({self.dockable_plot_widget_window}, {1}, QtCore.Qt.Horizontal)
        # disable and enable notifiers
        self.disable_notifier = GenericObservable()
        self.enable_notifier = GenericObservable()

        # disable and enable observers
        self.disable_observer = GenericObserver(self.disable_notifier.notify_subscribers)
        self.enable_observer = GenericObserver(self.enable_notifier.notify_subscribers)

        self.setup_dummy()

        self.setup_tabs()
        self.help_widget = HelpWidget("Elemental Analysis")

        central_widget = QtWidgets.QWidget()
        vertical_layout = QtWidgets.QVBoxLayout()
        vertical_layout.addWidget(self.load_widget.view)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle(self.context.name)

        # setup connections between notifiers and observers
        self.setup_enable_notifier()
        self.setup_disable_notifier()
        self.setup_load_observers()
        self.setup_gui_variable_observers()
        self.setup_group_calculation_enable_notifier()
        self.setup_group_calculation_disable_notifier()
        self.setup_grouping_changed_observers()
        self.setup_update_view_notifier()
        #self.setup_fitting_notifier()

        self.setMinimumHeight(800)

    def setup_dummy(self):
        self.load_widget = LoadWidget(self.loaded_data, self.context, parent=self)
        self.home_tab = QtWidgets.QLineEdit("home")
        self.grouping_tab_widget = EAGroupingTabWidget(self.context)
        self.fitting_tab = EAFittingTabWidget(self.context, self)
        self.auto_tab = EAAutoTabWidget(self.context)

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view, 'Grouping')
        self.tabs.addTabWithOrder(self.fitting_tab.fitting_tab_view, 'Fitting')
        self.tabs.addTabWithOrder(self.auto_tab.auto_tab_view, 'Automatic')
        self.tabs.set_slot_for_tab_changed(self.handle_tab_changed)

    def handle_tab_changed(self):
        index = self.tabs.currentIndex()
        # the plot mode indices are from the order the plots are stored
        if TAB_ORDER[index] in ["Home", "Grouping", "Automatic"]:  # Plot all the selected data
            plot_mode = self.plot_widget.data_index
        # Plot the displayed workspace
        #elif TAB_ORDER[index] in ["Fitting"]:
            #plot_mode = self.plot_widget.fit_index
        else:
            return
        self.plot_widget.set_plot_view(plot_mode)

    def closeEvent(self, event):
        self.removeDockWidget(self.dockable_plot_widget_window)
        self.context.ads_observer.unsubscribe()
        self.context.ads_observer = None
        self.tabs.closeEvent(event)
        super(ElementalAnalysisGui, self).closeEvent(event)

    def setup_disable_notifier(self):
        self.disable_notifier.add_subscriber(self.load_widget.load_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.grouping_tab_widget.grouping_table_view.disable_table_observer)

        self.disable_notifier.add_subscriber(self.auto_tab.auto_tab_presenter.disable_tab_observer)

        self.disable_notifier.add_subscriber(self.fitting_tab.fitting_tab_view.disable_tab_observer)

    def setup_enable_notifier(self):
        self.enable_notifier.add_subscriber(self.load_widget.load_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.grouping_tab_widget.grouping_table_view.enable_table_observer)

        self.enable_notifier.add_subscriber(self.auto_tab.auto_tab_presenter.enable_tab_observer)

        self.enable_notifier.add_subscriber(self.fitting_tab.fitting_tab_view.enable_tab_observer)

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.auto_tab.auto_tab_presenter.update_view_observer)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

        #self.fitting_tab.fitting_tab_presenter.selected_fit_results_changed.add_subscriber(
        #    self.plot_widget.fit_mode.plot_selected_fit_observer)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.grouping_table_widget.data_changed_notifier.add_subscriber(
            self.auto_tab.auto_tab_presenter.group_change_observer)

        self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.input_workspace_observer)

        for observer in self.plot_widget.data_changed_observers:
            self.grouping_tab_widget.grouping_table_widget.selected_group_changed_notifier.add_subscriber(observer)

        for observer in self.plot_widget.workspace_deleted_from_ads_observers:
            self.context.deleted_plots_notifier.add_subscriber(observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

    def setup_group_calculation_enable_notifier(self):
        self.load_widget.run_widget.enable_notifier.add_subscriber(self.enable_observer)

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
            self.enable_observer)

        self.context.calculation_finished_notifier.add_subscriber(self.enable_observer)

        self.auto_tab.auto_tab_presenter.model.calculation_finished_notifier.add_subscriber(self.enable_observer)

        self.fitting_tab.fitting_tab_presenter.enable_editing_notifier.add_subscriber(self.enable_observer)

    def setup_group_calculation_disable_notifier(self):
        self.load_widget.run_widget.disable_notifier.add_subscriber(self.disable_observer)

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
            self.disable_observer)

        self.context.calculation_started_notifier.add_subscriber(self.disable_observer)

        self.auto_tab.auto_tab_presenter.model.calculation_started_notifier.add_subscriber(self.disable_observer)

        self.fitting_tab.fitting_tab_presenter.disable_editing_notifier.add_subscriber(self.disable_observer)

    def setup_update_view_notifier(self):
        self.context.update_view_from_model_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.update_view_from_model_observer)

        self.context.update_view_from_model_notifier.add_subscriber(
            self.auto_tab.auto_tab_presenter.update_view_observer)

        self.context.update_view_from_model_notifier.add_subscriber(
            self.load_widget.load_widget.update_view_from_model_observer)

        self.context.update_view_from_model_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.input_workspace_observer)

        self.context.update_view_from_model_notifier.add_subscriber(
            self.fitting_tab.fitting_tab_presenter.update_view_from_model_observer)

    """def setup_fitting_notifier(self):
        #Connect fitting tab to inform of new fits

        self.fitting_tab.fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber(
            self.plot_widget.fit_mode.remove_plot_guess_observer)

        self.fitting_tab.fitting_tab_presenter.update_plot_guess_notifier.add_subscriber(
            self.plot_widget.fit_mode.update_plot_guess_observer)"""
