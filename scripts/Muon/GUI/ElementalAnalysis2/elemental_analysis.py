# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

import Muon.GUI.Common.message_box as message_box
from Muon.GUI.Common.help_widget.help_widget_presenter import HelpWidget
from Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext


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
        self.context = ElementalAnalysisContext()
        self.current_tab = ''

        self.setup_dummy()

        self.setup_tabs()
        self.help_widget = HelpWidget("Elemental Analysis")

        central_widget = QtWidgets.QWidget()
        vertical_layout = QtWidgets.QVBoxLayout()
        vertical_layout.addWidget(self.load_widget)
        vertical_layout.addWidget(self.tabs)
        vertical_layout.addWidget(self.help_widget.view)
        central_widget.setLayout(vertical_layout)

        self.setCentralWidget(central_widget)
        self.setWindowTitle(self.context.name)

    def setup_dummy(self):
        self.load_widget = QtWidgets.QLineEdit("load")
        self.home_tab = QtWidgets.QLineEdit("home")
        self.grouping_tab_widget = QtWidgets.QLineEdit("grouping")
        self.fitting_tab = QtWidgets.QLineEdit("fitting")

    def setup_tabs(self):
        """
        Set up the tabbing structure; the tabs work similarly to conventional
        web browsers.
        """
        self.tabs = DetachableTabWidget(self)
        self.tabs.addTabWithOrder(self.home_tab, 'Home')
        self.tabs.addTabWithOrder(self.grouping_tab_widget,
                                  'Grouping')
        self.tabs.addTabWithOrder(self.fitting_tab, 'Fitting')

    def closeEvent(self, event):
        self.tabs.closeEvent(event)
        super(ElementalAnalysisGui, self).closeEvent(event)
