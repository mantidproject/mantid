# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
import Muon.GUI.Common.message_box as message_box
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.load_widget.load_widget import LoadWidget
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_widget import EAGroupingTabWidget


class ElementalAnalysisGui(QtWidgets.QMainWindow):
    """
    The Elemental Analysis 2.0 interface.
    """

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setObjectName("ElementalAnalysis2")
        self.loaded_data = MuonLoadData()
        self.data_context = DataContext(self.loaded_data)
        self.group_context = EAGroupContext(self.data_context.check_group_contains_valid_detectors)
        self.gui_context = MuonGuiContext()
        self.context = ElementalAnalysisContext(self.group_context, self.gui_context)
        self.current_tab = ''

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

        self.setup_load_observers()

    def setup_dummy(self):
        self.load_widget = LoadWidget(self.loaded_data, self.context, parent=self)
        self.home_tab = QtWidgets.QLineEdit("home")
        self.grouping_tab_widget = EAGroupingTabWidget(self.context)
        self.fitting_tab = QtWidgets.QLineEdit("fitting")

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget.group_tab_view,
                                  'Grouping')
        self.tabs.addTabWithOrder(self.fitting_tab, 'Fitting')

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        super(ElementalAnalysisGui, self).closeEvent(event)

    def setup_disable_notifier(self):

        self.disable_notifier.add_subscriber(self.load_widget.load_widget.disable_observer)

        self.disable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.disable_tab_observer)

    def setup_enable_notifier(self):

        self.enable_notifier.add_subscriber(self.load_widget.load_widget.enable_observer)

        self.enable_notifier.add_subscriber(self.grouping_tab_widget.group_tab_presenter.enable_tab_observer)

    def setup_load_observers(self):
        self.load_widget.load_widget.loadNotifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.loadObserver)

    def setup_gui_variable_observers(self):
        self.context.gui_context.gui_variables_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.gui_variables_observer)

    def setup_grouping_changed_observers(self):
        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.home_tab.home_tab_widget.groupingObserver)

        self.grouping_tab_widget.group_tab_presenter.groupingNotifier.add_subscriber(
            self.phase_tab.phase_table_presenter.group_change_observer)

    def setup_on_load_enabler(self):
        self.load_widget.load_widget.load_run_widget.enable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.enable_observer)

    def setup_on_load_disabler(self):
        self.load_widget.load_widget.load_run_widget.disable_notifier.add_subscriber(
            self.grouping_tab_widget.group_tab_presenter.disable_observer)

    def setup_group_calculation_enable_notifier(self):

        self.grouping_tab_widget.group_tab_presenter.enable_editing_notifier.add_subscriber(
              self.enable_observer)

    def setup_group_calculation_disabler_notifier(self):

        self.grouping_tab_widget.group_tab_presenter.disable_editing_notifier.add_subscriber(
               self.disable_observer)
